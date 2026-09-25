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
AUTHORITY_SUCCESS_MARKERS = ("C client authority smoke completed",)
LISTEN_SERVER_AUTHORITY_SUCCESS_MARKER = "C listen-server authority smoke completed"
PIE_RESTART_SUCCESS_MARKER = "C PIE restart smoke completed"
MULTI_PIE_SUCCESS_MARKER = "C multi-PIE context smoke completed"
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
    "C PIE restart smoke failed",
    "C multi-PIE context smoke failed",
)
AUTHORITY_FAILURE_MARKERS = ("C client authority smoke failed", "LogPython: Error")


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


def configure_pie_settings(
    repo_root: Path, client_count: int, net_mode: str = "PIE_Client"
) -> tuple[Path, bytes | None]:
    """Temporarily set the per-project Editor options for a PIE network setup."""
    if client_count < 1:
        raise ValueError("PIE client_count must be positive")
    path = repo_root / "Saved/Config/MacEditor/EditorPerProjectUserSettings.ini"
    original = path.read_bytes() if path.is_file() else None
    text = original.decode("utf-8") if original is not None else ""
    section = "[/Script/UnrealEd.LevelEditorPlaySettings]"
    updates = {
        "PlayNetMode": net_mode,
        "RunUnderOneProcess": "True",
        "PlayNumberOfClients": str(client_count),
    }
    lines = text.splitlines()
    section_start = next((i for i, line in enumerate(lines) if line.strip() == section), None)
    if section_start is None:
        if lines and lines[-1].strip():
            lines.append("")
        lines.extend((section, *[f"{key}={value}" for key, value in updates.items()]))
    else:
        section_end = next(
            (i for i in range(section_start + 1, len(lines)) if lines[i].lstrip().startswith("[")),
            len(lines),
        )
        remaining = dict(updates)
        for index in range(section_start + 1, section_end):
            key, separator, _ = lines[index].partition("=")
            if separator and key.strip() in remaining:
                value = remaining.pop(key.strip())
                lines[index] = f"{key}={value}"
        lines[section_end:section_end] = [f"{key}={value}" for key, value in remaining.items()]
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")
    return path, original


def configure_authority_pie_settings(repo_root: Path) -> tuple[Path, bytes | None]:
    """Configure an in-process PIE client and dedicated server for authority checks."""
    return configure_pie_settings(repo_root, 1)


def configure_listen_server_pie_settings(repo_root: Path) -> tuple[Path, bytes | None]:
    """Temporarily configure a listen server and one in-process PIE client."""
    return configure_pie_settings(repo_root, 2, "PIE_ListenServer")


def configure_multi_pie_settings(repo_root: Path) -> tuple[Path, bytes | None]:
    """Temporarily configure two in-process PIE clients."""
    return configure_pie_settings(repo_root, 2)


def restore_authority_pie_settings(path: Path, original: bytes | None) -> None:
    """Restore the user's generated Editor settings after the authority run."""
    if original is None:
        path.unlink(missing_ok=True)
    else:
        path.write_bytes(original)


def run_smoke(
    engine_root: Path,
    timeout_seconds: float = 150.0,
    authority_only: bool = False,
    pie_restart_only: bool = False,
    multi_pie_only: bool = False,
    listen_server_only: bool = False,
) -> None:
    """Start PIE with NullRHI and require every C smoke marker."""
    if timeout_seconds <= 0:
        raise ValueError("timeout_seconds must be positive")
    if sum((authority_only, pie_restart_only, multi_pie_only, listen_server_only)) > 1:
        raise ValueError("authority, listen-server, PIE-restart, and multi-PIE modes cannot be combined")
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
    ]
    if authority_only or listen_server_only:
        command.extend((
            "-uec-tests-authority",
            "-uec-tests-exit",
            "-ExecCmds=py unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()",
        ))
        if listen_server_only:
            command.append("-uec-tests-listen-server")
            success_markers = (LISTEN_SERVER_AUTHORITY_SUCCESS_MARKER,)
        else:
            success_markers = AUTHORITY_SUCCESS_MARKERS
        failure_markers = AUTHORITY_FAILURE_MARKERS
    elif multi_pie_only:
        command.extend((
            "-uec-tests-multi-pie",
            "-uec-tests-exit",
            "-ExecCmds=py unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()",
        ))
        success_markers = (MULTI_PIE_SUCCESS_MARKER,)
        failure_markers = FAILURE_MARKERS
    else:
        command.extend((
            "-uec-tests-exit",
            "-ExecCmds=py unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()",
        ))
        if pie_restart_only:
            command.append("-uec-tests-pie-restart")
        success_markers = (
            (*SUCCESS_MARKERS, PIE_RESTART_SUCCESS_MARKER)
            if pie_restart_only else SUCCESS_MARKERS
        )
        failure_markers = FAILURE_MARKERS
    if authority_only:
        settings_snapshot = configure_authority_pie_settings(repo_root)
    elif listen_server_only:
        settings_snapshot = configure_listen_server_pie_settings(repo_root)
    elif multi_pie_only:
        settings_snapshot = configure_multi_pie_settings(repo_root)
    else:
        settings_snapshot = None
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
        if settings_snapshot is not None:
            restore_authority_pie_settings(*settings_snapshot)
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
        while len(completed) != len(success_markers):
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
                failed_marker = next((marker for marker in failure_markers if marker in line), None)
                if failed_marker is not None:
                    failure = f"Editor PIE reported smoke failure: {line}"
                    break
                completed.update(
                    marker for marker in success_markers if marker in line
                )
            if process.poll() is not None and len(completed) != len(success_markers):
                failure = (
                    f"Unreal Editor exited with status {process.returncode} "
                    "before the PIE smoke completed."
                )
                break
    finally:
        if failure is None:
            try:
                return_code = process.wait(timeout=15)
                if return_code != 0:
                    failure = (
                        f"Unreal Editor exited with status {return_code} "
                        "after the PIE smoke completed."
                    )
            except subprocess.TimeoutExpired:
                stop_process(process)
                failure = "Unreal Editor did not exit cleanly after the PIE smoke completed."
        else:
            stop_process(process)
        reader.join(timeout=5)
        if process.stdout is not None and not reader.is_alive():
            process.stdout.close()
        if settings_snapshot is not None:
            restore_authority_pie_settings(*settings_snapshot)

    if failure is not None:
        recent_output = "\n".join(tail)
        details = f"\nRecent Editor output:\n{recent_output}" if recent_output else ""
        raise RuntimeError(failure + details)


def main(argv: list[str]) -> int:
    if len(argv) not in (2, 3) or (
        len(argv) == 3 and argv[2] not in (
            "--authority-only", "--listen-server-only", "--pie-restart-only", "--multi-pie-only"
        )
    ):
        print(
            f"Usage: {Path(argv[0]).name} <unreal-engine-root> "
            "[--authority-only | --listen-server-only | --pie-restart-only | --multi-pie-only]",
            file=sys.stderr,
        )
        return 2
    try:
        authority_only = len(argv) == 3 and argv[2] == "--authority-only"
        listen_server_only = len(argv) == 3 and argv[2] == "--listen-server-only"
        pie_restart_only = len(argv) == 3 and argv[2] == "--pie-restart-only"
        multi_pie_only = len(argv) == 3 and argv[2] == "--multi-pie-only"
        run_smoke(
            Path(argv[1]), authority_only=authority_only,
            pie_restart_only=pie_restart_only, multi_pie_only=multi_pie_only,
            listen_server_only=listen_server_only,
        )
    except (OSError, RuntimeError, ValueError) as error:
        print(error, file=sys.stderr)
        return 1
    if authority_only:
        print("Editor multiplayer PIE completed the client-world physics authority smoke check.")
    elif listen_server_only:
        print("Editor listen-server PIE completed server and client authority smoke checks.")
    elif pie_restart_only:
        print("Editor PIE restart smoke verified stale-handle rejection and world-tick cleanup across two sessions.")
    elif multi_pie_only:
        print("Editor PIE smoke verified explicit world lookup and distinct instance ids across two clients.")
    else:
        print(
            "Editor PIE completed the C bootstrap, reflection metadata, containers, "
            "Blueprint invocation, "
            "persistence/configuration, collision, "
            "GC lifetime, physics, event, latent, Enhanced Input, queue, "
            "async save/load, async object load, gameplay, and travel smoke checks."
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
