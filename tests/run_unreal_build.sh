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
if ! engine_version=$(python3 "$repo_dir/tests/unreal_version.py" \
    "$repo_dir/UnrealCAPIHost.uproject" "$engine_version_file" \
    "$repo_dir/UE_TARGET_VERSION"); then
    if [ "${UEC_ALLOW_ENGINE_MISMATCH:-0}" != 1 ]; then
        printf '%s\n' 'Set UEC_ALLOW_ENGINE_MISMATCH=1 only for an explicit compatibility probe.' >&2
        exit 2
    fi
    printf '%s\n' 'Unreal engine metadata or target version is invalid.' >&2
    exit 2
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

printf 'Unreal %s %s build, cook, stage, and package completed.\n' "$platform" "$configuration"
if [ "$platform" = "$host_platform" ] && [ "$configuration" = Development ]; then
    python3 "$repo_dir/tests/unreal_runtime.py" "$build_dir/archive"
else
    printf 'Packaged runtime smoke skipped for %s %s target on %s host.\n' \
        "$platform" "$configuration" "$host_platform"
fi
