#!/usr/bin/env python3
"""Run the host's C ABI smoke checks in an Unreal Editor PIE world."""

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
    "C game-thread queue smoke completed",
    "C async save smoke completed",
    "C async object load smoke completed",
    "C gameplay example smoke completed",
    "C travel smoke completed",
)
FAILURE_MARKERS = (
    "C consumer bootstrap failed",
    "C event bridge smoke failed",
    "C latent invocation smoke failed",
    "C game-thread queue smoke failed",
    "C async save smoke failed",
    "C async object load smoke failed",
    "C gameplay example smoke failed",
    "C travel smoke failed",
)


def find_editor_executable(engine_root: Path) -> Path:
    """Resolve the installed Editor executable for the current host."""
    if sys.platform == "darwin":
        relative_path = Path("Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor")
    elif os.name == "nt":
        relative_path = Path("Engine/Binaries/Win64/UnrealEditor.exe")
    elif sys.platform.startswith("linux"):
        relative_path = Path("Engine/Binaries/Linux/UnrealEditor")
    else:
        raise RuntimeError(f"Unsupported host platform for Editor PIE: {sys.platform}")
    executable = engine_root / relative_path
    if not executable.is_file():
        raise RuntimeError(f"Unreal Editor executable was not found: {executable}")
    return executable


def stop_process(process: subprocess.Popen[str]) -> None:
    """Stop the disposable Editor process after success or failure."""
    if process.poll() is not None:
        return
    process.terminate()
    try:
        process.wait(timeout=5)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait(timeout=10)


def run_smoke(engine_root: Path, timeout_seconds: float = 150.0) -> None:
    """Start PIE with NullRHI and require every C smoke marker."""
    if timeout_seconds <= 0:
        raise ValueError("timeout_seconds must be positive")
    repo_root = Path(__file__).resolve().parent.parent
    executable = find_editor_executable(engine_root)
    command = [
        str(executable),
        str(repo_root / "UnrealCAPIHost.uproject"),
        "-nullrhi",
        "-nosound",
        "-unattended",
        "-nosplash",
        "-nop4",
        "-stdout",
        "-FullStdOutLogOutput",
        "-ExecCmds=py unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()",
    ]
    try:
        process = subprocess.Popen(
            command,
            cwd=repo_root,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            errors="replace",
            bufsize=1,
        )
    except OSError as error:
        raise RuntimeError(f"Could not launch Unreal Editor {executable}: {error}") from error

    output: queue.Queue[str | None] = queue.Queue()

    def read_output() -> None:
        assert process.stdout is not None
        for line in process.stdout:
            output.put(line.rstrip("\r\n"))
        output.put(None)

    reader = threading.Thread(target=read_output, daemon=True)
    reader.start()
    tail: deque[str] = deque(maxlen=50)
    completed: set[str] = set()
    deadline = time.monotonic() + timeout_seconds
    failure: str | None = None
    try:
        while len(completed) != len(SUCCESS_MARKERS):
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                failure = f"Editor PIE did not complete smoke checks within {timeout_seconds:g}s."
                break
            try:
                line = output.get(timeout=min(0.1, remaining))
            except queue.Empty:
                line = ""
            if line is None:
                failure = (
                    f"Unreal Editor exited with status {process.returncode} "
                    "before the PIE smoke completed."
                )
                break
            if line:
                tail.append(line)
                failed_marker = next(
                    (marker for marker in FAILURE_MARKERS if marker in line), None
                )
                if failed_marker is not None:
                    failure = f"Editor PIE reported smoke failure: {line}"
                    break
                completed.update(
                    marker for marker in SUCCESS_MARKERS if marker in line
                )
            if process.poll() is not None and len(completed) != len(SUCCESS_MARKERS):
                failure = (
                    f"Unreal Editor exited with status {process.returncode} "
                    "before the PIE smoke completed."
                )
                break
    finally:
        stop_process(process)
        reader.join(timeout=1)
        if process.stdout is not None:
            process.stdout.close()

    if failure is not None:
        recent_output = "\n".join(tail)
        details = f"\nRecent Editor output:\n{recent_output}" if recent_output else ""
        raise RuntimeError(failure + details)


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(f"Usage: {Path(argv[0]).name} <unreal-engine-root>", file=sys.stderr)
        return 2
    try:
        run_smoke(Path(argv[1]))
    except (OSError, RuntimeError, ValueError) as error:
        print(error, file=sys.stderr)
        return 1
    print(
        "Editor PIE completed the C bootstrap, event, latent, queue, async save/load, "
        "async object load, gameplay, and travel smoke checks."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
