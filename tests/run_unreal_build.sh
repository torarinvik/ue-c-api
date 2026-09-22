#!/usr/bin/env sh
set -eu

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
engine_root=${UE_ROOT:-}
configuration=${UEC_UNREAL_CONFIGURATION:-Development}
requested_platform=${UEC_UNREAL_PLATFORM:-}
skip_editor_args=

sh "$repo_dir/tests/run_checks.sh"

if [ -z "$engine_root" ]; then
    printf '%s\n' 'UE_ROOT must point to an Unreal Engine installation.' >&2
    exit 2
fi
if [ ! -d "$engine_root/Engine" ]; then
    printf 'UE_ROOT does not contain an Engine directory: %s\n' "$engine_root" >&2
    exit 2
fi

engine_version_file="$engine_root/Engine/Build/Build.version"
if [ ! -f "$engine_version_file" ]; then
    printf 'Unreal engine version metadata was not found: %s\n' "$engine_version_file" >&2
    exit 2
fi
if ! engine_version=$(python3 - "$repo_dir/UnrealCAPIHost.uproject" "$engine_version_file" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as project_file:
    project = json.load(project_file)
with open(sys.argv[2], encoding="utf-8") as version_file:
    version = json.load(version_file)

association = str(project.get("EngineAssociation", ""))
parts = association.split(".")
if len(parts) < 2 or not all(part.isdigit() for part in parts[:2]):
    raise SystemExit("project EngineAssociation must contain major and minor numbers")
major = int(version["MajorVersion"])
minor = int(version["MinorVersion"])
patch = int(version.get("PatchVersion", 0))
if (major, minor) != (int(parts[0]), int(parts[1])):
    raise SystemExit(
        f"engine {major}.{minor}.{patch} does not match project EngineAssociation {association}"
    )
print(f"{major}.{minor}.{patch}")
PY
); then
    if [ "${UEC_ALLOW_ENGINE_MISMATCH:-0}" != 1 ]; then
        printf '%s\n' "$engine_version" >&2
        printf '%s\n' 'Set UEC_ALLOW_ENGINE_MISMATCH=1 only for an explicit compatibility probe.' >&2
        exit 2
    fi
    engine_version=$(python3 - "$engine_version_file" <<'PY'
import json
import sys
with open(sys.argv[1], encoding="utf-8") as version_file:
    version = json.load(version_file)
print("%s.%s.%s" % (version["MajorVersion"], version["MinorVersion"], version.get("PatchVersion", 0)))
PY
)
fi
printf 'Using Unreal Engine %s (%s configuration).\n' "$engine_version" "$configuration"

case "$(uname -s)" in
    Darwin) host_platform=Mac; platform=$host_platform; uat="$engine_root/Engine/Build/BatchFiles/RunUAT.sh" ;;
    Linux) host_platform=Linux; platform=$host_platform; uat="$engine_root/Engine/Build/BatchFiles/RunUAT.sh" ;;
    MINGW*|MSYS*|CYGWIN*) host_platform=Win64; platform=$host_platform; uat="$engine_root/Engine/Build/BatchFiles/RunUAT.bat" ;;
    *) printf 'Unsupported host platform: %s\n' "$(uname -s)" >&2; exit 2 ;;
esac
if [ -n "$requested_platform" ]; then
    case "$requested_platform" in
        *[!A-Za-z0-9]*)
            printf 'UEC_UNREAL_PLATFORM contains unsupported characters: %s\n' "$requested_platform" >&2
            exit 2
            ;;
        *)
            platform=$requested_platform
            if [ "$platform" != "$host_platform" ]; then
                skip_editor_args=-skipbuildeditor
            fi
            ;;
    esac
fi
if [ ! -f "$uat" ]; then
    printf 'Unreal Automation Tool was not found: %s\n' "$uat" >&2
    exit 2
fi

build_dir=$(mktemp -d "${TMPDIR:-/tmp}/uec-unreal-build.XXXXXX")
trap 'rm -rf "$build_dir"' EXIT HUP INT TERM

"$uat" BuildCookRun \
    -project="$repo_dir/UnrealCAPIHost.uproject" \
    -noP4 -utf8output -unattended \
    -platform="$platform" -clientconfig="$configuration" \
    -build -cook -stage -pak -archive \
    -archivedirectory="$build_dir/archive" \
    $skip_editor_args

printf 'Unreal %s %s build and cook completed.\n' "$platform" "$configuration"
