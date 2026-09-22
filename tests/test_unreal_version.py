#!/usr/bin/env python3
"""Tests for the Unreal engine patch gate."""

import pathlib
import unittest

from unreal_version import validate_engine_version


class UnrealVersionTests(unittest.TestCase):
    def setUp(self):
        self.project = {"EngineAssociation": "5.8"}
        self.target = "5.8.3"

    def test_target_patch_is_accepted(self):
        self.assertEqual(
            validate_engine_version(
                self.project, {"MajorVersion": 5, "MinorVersion": 8, "PatchVersion": 3},
                self.target,
            ),
            "5.8.3",
        )

    def test_newer_patch_is_accepted(self):
        self.assertEqual(
            validate_engine_version(
                self.project, {"MajorVersion": 5, "MinorVersion": 8, "PatchVersion": 4},
                self.target,
            ),
            "5.8.4",
        )

    def test_older_patch_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "older than the target"):
            validate_engine_version(
                self.project, {"MajorVersion": 5, "MinorVersion": 8, "PatchVersion": 2},
                self.target,
            )

    def test_engine_association_mismatch_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "EngineAssociation"):
            validate_engine_version(
                self.project, {"MajorVersion": 5, "MinorVersion": 7, "PatchVersion": 4},
                self.target,
            )

    def test_target_must_match_project_descriptor(self):
        with self.assertRaisesRegex(ValueError, "UE_TARGET_VERSION"):
            validate_engine_version(
                {"EngineAssociation": "5.7"},
                {"MajorVersion": 5, "MinorVersion": 7, "PatchVersion": 4},
                self.target,
            )

    def test_explicit_compatibility_override_bypasses_target_checks(self):
        self.assertEqual(
            validate_engine_version(
                self.project, {"MajorVersion": 5, "MinorVersion": 7, "PatchVersion": 4},
                self.target, allow_mismatch=True,
            ),
            "5.7.4",
        )

    def test_repository_target_is_pinned_to_latest_verified_hotfix(self):
        root = pathlib.Path(__file__).resolve().parents[1]
        self.assertEqual((root / "UE_TARGET_VERSION").read_text().strip(), "5.8.3")


if __name__ == "__main__":
    unittest.main()
