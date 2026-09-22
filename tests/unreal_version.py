#!/usr/bin/env python3
"""Validate the installed Unreal patch against the project target."""

import json
import os
import sys


def parse_version(value):
    parts = value.strip().split(".")
    if len(parts) != 3 or not all(part.isdigit() for part in parts):
        raise ValueError("target engine version must use major.minor.patch numbers")
    return tuple(int(part) for part in parts)


def validate_engine_version(project, build, target_text, allow_mismatch=False):
    association = str(project.get("EngineAssociation", "")).split(".")
    if len(association) < 2 or not all(part.isdigit() for part in association[:2]):
        raise ValueError("project EngineAssociation must contain major and minor numbers")
    project_version = (int(association[0]), int(association[1]))
    target_version = parse_version(target_text)
    engine_version = (
        int(build["MajorVersion"]),
        int(build["MinorVersion"]),
        int(build.get("PatchVersion", 0)),
    )

    if not allow_mismatch:
        if project_version != target_version[:2]:
            raise ValueError("UE_TARGET_VERSION major/minor must match EngineAssociation")
        if engine_version[:2] != project_version:
            raise ValueError(
                "engine %s.%s.%s does not match project EngineAssociation %s"
                % (*engine_version, ".".join(association[:2]))
            )
        if engine_version[2] < target_version[2]:
            raise ValueError(
                "engine %s.%s.%s is older than the target Unreal version %s"
                % (*engine_version, ".".join(str(part) for part in target_version))
            )

    return ".".join(str(part) for part in engine_version)


def main(argv):
    if len(argv) != 4:
        print("usage: unreal_version.py PROJECT BUILD_VERSION TARGET_VERSION", file=sys.stderr)
        return 2
    try:
        with open(argv[1], encoding="utf-8") as project_file:
            project = json.load(project_file)
        with open(argv[2], encoding="utf-8") as build_file:
            build = json.load(build_file)
        with open(argv[3], encoding="utf-8") as target_file:
            target_text = target_file.read()
        version = validate_engine_version(
            project, build, target_text,
            allow_mismatch=os.environ.get("UEC_ALLOW_ENGINE_MISMATCH") == "1",
        )
    except (OSError, ValueError, KeyError, json.JSONDecodeError) as error:
        print(str(error), file=sys.stderr)
        return 1
    print(version)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
