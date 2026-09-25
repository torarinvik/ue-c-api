#!/usr/bin/env python3
"""Run the packaged host and require its C ABI smoke checks to complete."""

from __future__ import annotations

import os
import queue
import subprocess
import sys
import threading
import time
from collections import deque
from pathlib import Path


SUCCESS_MARKERS = (
    "C consumer bootstrap completed",
    "C cooked reflection metadata smoke completed",
    "C persistence and configuration smoke completed",
    "C reflected container smoke completed",
    "C Blueprint invocation smoke completed",
    "C collision smoke completed",
    "C GC lifetime smoke completed",
    "C physics smoke completed",
    "C event bridge smoke completed",
    "C latent invocation smoke completed",
    "C Enhanced Input smoke completed",
    "C game-thread queue smoke completed",
    "C async save smoke completed",
    "C async object load smoke completed",
    "C gameplay example smoke completed",
    "C travel smoke completed",
)
SHUTDOWN_PENDING_SUCCESS_MARKER = "C shutdown pending-work smoke completed"
SHUTDOWN_REGISTRIES_SUCCESS_MARKER = "UEC runtime shutdown registries cleared"
FAILURE_MARKERS = (
    "C consumer bootstrap failed",
    "C cooked reflection metadata smoke failed",
    "C persistence and configuration smoke failed",
    "C reflected container smoke failed",
    "C Blueprint invocation smoke failed",
    "C collision smoke failed",
    "C GC lifetime smoke failed",
    "C physics smoke failed",
    "C physics smoke failed to start",
    "C event bridge smoke failed",
    "C latent invocation smoke failed",
    "C Enhanced Input smoke failed",
    "C game-thread queue smoke failed",
    "C async save smoke failed",
    "C async object load smoke failed",
    "C gameplay example smoke failed",
    "C travel smoke failed",
    "C shutdown pending-work smoke failed",
    "UEC runtime shutdown drain failed",
)


def find_host_executable(archive: Path) -> Path:
    """Find the packaged host executable across UAT archive layouts."""
    if archive.is_file():
        return archive
    name = "UnrealCAPIHost.exe" if os.name == "nt" else "UnrealCAPIHost"
    candidates = [path for path in archive.rglob(name) if path.is_file()]
    if len(candidates) != 1:
        rendered = ", ".join(str(path) for path in candidates) or "none"
        raise RuntimeError(f"Expected one packaged {name} under {archive}; found {rendered}.")
    return candidates[0]


def _stop_process(process: subprocess.Popen[str]) -> None:
    if process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()


def run_smoke(
    executable: Path,
    timeout_seconds: float = 90.0,
    startup_only: bool = False,
    graceful_exit: bool = False,
    shutdown_pending_only: bool = False,
) -> None:
    """Launch a packaged host and require smoke markers or sustained startup."""
    if timeout_seconds <= 0:
        raise ValueError("timeout_seconds must be positive")
    if startup_only and shutdown_pending_only:
        raise ValueError("startup-only and shutdown-pending-only modes cannot be combined")
    executable = executable.resolve()
    command = [
        str(executable),
        "-unattended",
        "-nosplash",
        "-nop4",
        "-nullrhi",
        "-nosound",
        "-stdout",
        "-FullStdOutLogOutput",
    ]
    if not startup_only:
        command.append("-uec-tests-exit")
    if shutdown_pending_only:
        command.append("-uec-tests-shutdown-pending")
    success_markers = (
        (SHUTDOWN_PENDING_SUCCESS_MARKER, SHUTDOWN_REGISTRIES_SUCCESS_MARKER)
        if shutdown_pending_only else SUCCESS_MARKERS
    )
    try:
        process = subprocess.Popen(
            command,
            cwd=executable.parent,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            errors="replace",
            bufsize=1,
        )
    except OSError as error:
        raise RuntimeError(f"Could not launch packaged host {executable}: {error}") from error

    output: queue.Queue[str | None] = queue.Queue()

    def read_output() -> None:
        assert process.stdout is not None
        for line in process.stdout:
            output.put(line.rstrip("\r\n"))
        output.put(None)

    reader = threading.Thread(target=read_output, daemon=True)
    reader.start()
    tail: deque[str] = deque(maxlen=40)
    completed: set[str] = set()
    deadline = time.monotonic() + timeout_seconds
    failure: str | None = None
    try:
        startup_deadline = time.monotonic() + min(10.0, timeout_seconds)
        while startup_only and time.monotonic() < startup_deadline:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                failure = "Packaged host did not remain running through its startup check."
                break
            try:
                line = output.get(timeout=min(0.1, remaining))
            except queue.Empty:
                continue
            if line is None:
                failure = (
                    f"Packaged host exited with status {process.returncode} "
                    "during its startup check."
                )
                break
            if line:
                tail.append(line)
            if process.poll() is not None:
                failure = (
                    f"Packaged host exited with status {process.returncode} "
                    "during its startup check."
                )
                break
        while not startup_only and len(completed) != len(success_markers):
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                failure = f"Packaged host did not complete smoke checks within {timeout_seconds:g}s."
                break
            try:
                line = output.get(timeout=min(0.1, remaining))
            except queue.Empty:
                line = ""
            if line:
                tail.append(line)
                failed_marker = next((marker for marker in FAILURE_MARKERS if marker in line), None)
                if failed_marker is not None:
                    failure = f"Packaged host reported smoke failure: {line}"
                    break
                completed.update(marker for marker in success_markers if marker in line)
            if process.poll() is not None and len(completed) != len(success_markers):
                failure = f"Packaged host exited with status {process.returncode} before smoke completion."
                break
    finally:
        if failure is None and graceful_exit and not startup_only:
            try:
                process.wait(timeout=15)
            except subprocess.TimeoutExpired:
                _stop_process(process)
                failure = "Packaged host did not exit after its final smoke marker."
            else:
                if process.returncode != 0:
                    failure = (
                        "Packaged host exited with status "
                        f"{process.returncode} after its final smoke marker."
                    )
        else:
            _stop_process(process)
        reader.join(timeout=5)
        if process.stdout is not None and not reader.is_alive():
            process.stdout.close()

    if failure is not None:
        recent_output = "\n".join(tail)
        details = f"\nRecent host output:\n{recent_output}" if recent_output else ""
        raise RuntimeError(failure + details)


def main(argv: list[str]) -> int:
    if len(argv) not in (2, 3) or (
        len(argv) == 3 and argv[2] not in ("--startup-only", "--shutdown-pending-only")
    ):
        print(
            f"Usage: {Path(argv[0]).name} <packaged-archive-or-executable> "
            "[--startup-only | --shutdown-pending-only]",
            file=sys.stderr,
        )
        return 2
    archive = Path(argv[1])
    startup_only = len(argv) == 3 and argv[2] == "--startup-only"
    shutdown_pending_only = len(argv) == 3 and argv[2] == "--shutdown-pending-only"
    try:
        executable = find_host_executable(archive)
        run_smoke(
            executable,
            startup_only=startup_only,
            shutdown_pending_only=shutdown_pending_only,
            graceful_exit=not startup_only,
        )
    except RuntimeError as error:
        print(error, file=sys.stderr)
        return 1
    if startup_only:
        print("Packaged host remained running through its Shipping startup check.")
        return 0
    if shutdown_pending_only:
        print(
            "Packaged host drained pending work and its native event delegate without running "
            "queue, save, asset-load, or event-bridge callbacks during shutdown."
        )
        return 0
    print(
        "Packaged Development host completed the C bootstrap, reflection metadata, "
        "containers, Blueprint invocation, persistence/configuration, collision, "
        "GC lifetime, physics, event, latent, "
        "Enhanced Input, queue, async save/load, async object load, gameplay, and "
        "travel smoke checks."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
