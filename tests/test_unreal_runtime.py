#!/usr/bin/env python3
"""Portable tests for locating and monitoring the packaged host smoke run."""

import io
import os
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from unreal_runtime import find_host_executable, run_smoke
from unreal_pie_runtime import (
    configure_authority_pie_settings,
    restore_authority_pie_settings,
    SUCCESS_MARKERS,
    run_smoke as run_editor_smoke,
)


class UnrealRuntimeTests(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.root = Path(self.temp_dir.name)

    def tearDown(self):
        self.temp_dir.cleanup()

    def make_host(self, output):
        name = "UnrealCAPIHost.exe" if os.name == "nt" else "UnrealCAPIHost"
        executable = self.root / "Archive" / name
        executable.parent.mkdir(parents=True)
        script = "#!/usr/bin/env python3\n" + "\n".join(
            f"print({line!r}, flush=True)" for line in output
        )
        script += "\nimport time\ntime.sleep(30)\n"
        executable.write_text(script, encoding="utf-8")
        executable.chmod(0o755)
        return executable

    def test_finds_packaged_executable(self):
        executable = self.make_host([])
        self.assertEqual(find_host_executable(self.root / "Archive"), executable)

    def test_authority_pie_settings_are_restored(self):
        settings_path = self.root / "Saved/Config/MacEditor/EditorPerProjectUserSettings.ini"
        settings_path.parent.mkdir(parents=True)
        original = (
            b"[/Script/UnrealEd.LevelEditorPlaySettings]\n"
            b"PlayNetMode=PIE_Standalone\n"
            b"RunUnderOneProcess=False\n"
            b"PlayNumberOfClients=4\n"
            b"[/Script/UnrealEd.EditorPerProjectUserSettings]\n"
            b"KeepThis=True\n"
        )
        settings_path.write_bytes(original)
        path, snapshot = configure_authority_pie_settings(self.root)
        configured = path.read_text(encoding="utf-8")
        self.assertIn("PlayNetMode=PIE_Client", configured)
        self.assertIn("RunUnderOneProcess=True", configured)
        self.assertIn("PlayNumberOfClients=1", configured)
        restore_authority_pie_settings(path, snapshot)
        self.assertEqual(path.read_bytes(), original)

    def test_authority_pie_settings_remove_temporary_file(self):
        path, snapshot = configure_authority_pie_settings(self.root)
        self.assertTrue(path.exists())
        restore_authority_pie_settings(path, snapshot)
        self.assertFalse(path.exists())

    def test_requires_unique_executable(self):
        with self.assertRaisesRegex(RuntimeError, "found none"):
            find_host_executable(self.root)

    def test_accepts_all_smoke_markers(self):
        executable = self.make_host([
            "C consumer bootstrap completed",
            "C collision smoke completed",
            "C GC lifetime smoke completed",
            "C physics smoke completed",
            "C event bridge smoke completed",
            "C latent invocation smoke completed",
            "C game-thread queue smoke completed",
            "C async save smoke completed",
            "C async object load smoke completed",
            "C gameplay example smoke completed",
            "C travel smoke completed",
        ])
        run_smoke(executable, timeout_seconds=10.0)

    def test_surfaces_smoke_failure(self):
        executable = self.make_host(["C event bridge smoke failed with result 8"])
        with self.assertRaisesRegex(RuntimeError, "C event bridge smoke failed"):
            run_smoke(executable, timeout_seconds=10.0)

    @patch("unreal_pie_runtime.find_editor_executable", return_value=Path("/fake/UnrealEditor"))
    @patch("unreal_pie_runtime.subprocess.Popen")
    def test_surfaces_editor_crash_after_success_markers(self, popen, _find_editor):
        process = popen.return_value
        process.stdout = io.StringIO("\n".join(SUCCESS_MARKERS) + "\n")
        process.poll.return_value = None
        process.wait.return_value = -6

        with self.assertRaisesRegex(RuntimeError, "exited with status -6 after the PIE smoke completed"):
            run_editor_smoke(Path("/fake/engine"), timeout_seconds=1.0)

    def test_surfaces_collision_smoke_failure(self):
        executable = self.make_host(["C collision smoke failed with result 8"])
        with self.assertRaisesRegex(RuntimeError, "C collision smoke failed"):
            run_smoke(executable, timeout_seconds=10.0)

    def test_surfaces_gc_lifetime_smoke_failure(self):
        executable = self.make_host(["C GC lifetime smoke failed with result 8"])
        with self.assertRaisesRegex(RuntimeError, "C GC lifetime smoke failed"):
            run_smoke(executable, timeout_seconds=10.0)

    def test_surfaces_physics_smoke_failure(self):
        executable = self.make_host(["C physics smoke failed with result 8"])
        with self.assertRaisesRegex(RuntimeError, "C physics smoke failed"):
            run_smoke(executable, timeout_seconds=10.0)

    def test_surfaces_travel_smoke_failure(self):
        executable = self.make_host(["C travel smoke failed with result 8"])
        with self.assertRaisesRegex(RuntimeError, "C travel smoke failed"):
            run_smoke(executable, timeout_seconds=10.0)

    def test_surfaces_gameplay_example_smoke_failure(self):
        executable = self.make_host(["C gameplay example smoke failed with result 8"])
        with self.assertRaisesRegex(RuntimeError, "C gameplay example smoke failed"):
            run_smoke(executable, timeout_seconds=10.0)

    def test_surfaces_game_thread_queue_smoke_failure(self):
        executable = self.make_host(["C game-thread queue smoke failed with result 8"])
        with self.assertRaisesRegex(RuntimeError, "C game-thread queue smoke failed"):
            run_smoke(executable, timeout_seconds=10.0)

    def test_surfaces_async_save_smoke_failure(self):
        executable = self.make_host(["C async save smoke failed with result 8"])
        with self.assertRaisesRegex(RuntimeError, "C async save smoke failed"):
            run_smoke(executable, timeout_seconds=10.0)

    def test_surfaces_async_object_load_smoke_failure(self):
        executable = self.make_host(["C async object load smoke failed with result 8"])
        with self.assertRaisesRegex(RuntimeError, "C async object load smoke failed"):
            run_smoke(executable, timeout_seconds=10.0)

    def test_times_out_if_smoke_never_finishes(self):
        executable = self.make_host([])
        with self.assertRaisesRegex(RuntimeError, "within 0.1s"):
            run_smoke(executable, timeout_seconds=0.1)


if __name__ == "__main__":
    unittest.main()
