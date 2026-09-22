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
    "C event bridge smoke completed",
    "C latent invocation smoke completed",
)
FAILURE_MARKERS = (
    "C consumer bootstrap failed",
    "C event bridge smoke failed",
    "C latent invocation smoke failed",
)


def find_host_executable(archive: Path) -> Path:
    """Find the packaged host executable across UAT archive layouts."""
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


def run_smoke(executable: Path, timeout_seconds: float = 90.0) -> None:
    """Launch a packaged Development host and require every C smoke marker."""
    if timeout_seconds <= 0:
        raise ValueError("timeout_seconds must be positive")
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
        while len(completed) != len(SUCCESS_MARKERS):
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
                completed.update(marker for marker in SUCCESS_MARKERS if marker in line)
            if process.poll() is not None and len(completed) != len(SUCCESS_MARKERS):
                failure = f"Packaged host exited with status {process.returncode} before smoke completion."
                break
    finally:
        _stop_process(process)
        reader.join(timeout=1)
        if process.stdout is not None:
            process.stdout.close()

    if failure is not None:
        recent_output = "\n".join(tail)
        details = f"\nRecent host output:\n{recent_output}" if recent_output else ""
        raise RuntimeError(failure + details)


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(f"Usage: {Path(argv[0]).name} <packaged-archive>", file=sys.stderr)
        return 2
    archive = Path(argv[1])
    try:
        executable = find_host_executable(archive)
        run_smoke(executable)
    except RuntimeError as error:
        print(error, file=sys.stderr)
        return 1
    print("Packaged Development host completed the C bootstrap, event, and latent smoke checks.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
