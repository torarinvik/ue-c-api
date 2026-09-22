#!/usr/bin/env sh
set -eu

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
engine_root=${UE_ROOT:-}
configuration=${UEC_UNREAL_CONFIGURATION:-Development}
requested_platform=${UEC_UNREAL_PLATFORM:-}

if [ -z "$engine_root" ]; then
    printf '%s\n' 'UE_ROOT must point to an Unreal Engine installation.' >&2
    exit 2
fi
if [ ! -d "$engine_root/Engine" ]; then
    printf 'UE_ROOT does not contain an Engine directory: %s\n' "$engine_root" >&2
    exit 2
fi

case "$(uname -s)" in
    Darwin) platform=Mac; uat="$engine_root/Engine/Build/BatchFiles/RunUAT.sh" ;;
    Linux) platform=Linux; uat="$engine_root/Engine/Build/BatchFiles/RunUAT.sh" ;;
    MINGW*|MSYS*|CYGWIN*) platform=Win64; uat="$engine_root/Engine/Build/BatchFiles/RunUAT.bat" ;;
    *) printf 'Unsupported host platform: %s\n' "$(uname -s)" >&2; exit 2 ;;
esac
if [ -n "$requested_platform" ]; then
    case "$requested_platform" in
        *[!A-Za-z0-9]*)
            printf 'UEC_UNREAL_PLATFORM contains unsupported characters: %s\n' "$requested_platform" >&2
            exit 2
            ;;
        *) platform=$requested_platform ;;
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
    -archivedirectory="$build_dir/archive"

printf 'Unreal %s %s build and cook completed.\n' "$platform" "$configuration"
