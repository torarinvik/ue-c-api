#!/usr/bin/env python3
"""Portable tests for locating and monitoring the packaged host smoke run."""

import os
import tempfile
import unittest
from pathlib import Path

from unreal_runtime import find_host_executable, run_smoke


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

    def test_requires_unique_executable(self):
        with self.assertRaisesRegex(RuntimeError, "found none"):
            find_host_executable(self.root)

    def test_accepts_all_smoke_markers(self):
        executable = self.make_host([
            "C consumer bootstrap completed",
            "C event bridge smoke completed",
            "C latent invocation smoke completed",
            "C game-thread queue smoke completed",
            "C gameplay example smoke completed",
            "C travel smoke completed",
        ])
        run_smoke(executable, timeout_seconds=2.0)

    def test_surfaces_smoke_failure(self):
        executable = self.make_host(["C event bridge smoke failed with result 8"])
        with self.assertRaisesRegex(RuntimeError, "C event bridge smoke failed"):
            run_smoke(executable, timeout_seconds=2.0)

    def test_surfaces_travel_smoke_failure(self):
        executable = self.make_host(["C travel smoke failed with result 8"])
        with self.assertRaisesRegex(RuntimeError, "C travel smoke failed"):
            run_smoke(executable, timeout_seconds=2.0)

    def test_surfaces_gameplay_example_smoke_failure(self):
        executable = self.make_host(["C gameplay example smoke failed with result 8"])
        with self.assertRaisesRegex(RuntimeError, "C gameplay example smoke failed"):
            run_smoke(executable, timeout_seconds=2.0)

    def test_surfaces_game_thread_queue_smoke_failure(self):
        executable = self.make_host(["C game-thread queue smoke failed with result 8"])
        with self.assertRaisesRegex(RuntimeError, "C game-thread queue smoke failed"):
            run_smoke(executable, timeout_seconds=2.0)

    def test_times_out_if_smoke_never_finishes(self):
        executable = self.make_host([])
        with self.assertRaisesRegex(RuntimeError, "within 0.1s"):
            run_smoke(executable, timeout_seconds=0.1)


if __name__ == "__main__":
    unittest.main()
