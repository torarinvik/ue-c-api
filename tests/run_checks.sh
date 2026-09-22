#!/usr/bin/env sh
set -eu

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
public_dir="$repo_dir/Source/UnrealCAPI/Public"
consumer="$repo_dir/tests/c_smoke/c_smoke.c"
compat_consumer="$repo_dir/tests/c_smoke/c_compat.c"
host_stub="$repo_dir/tests/c_smoke/c_host_stub.c"
gameplay_example="$repo_dir/examples/c_gameplay/c_gameplay.c"
private_dir="$repo_dir/Source/UnrealCAPI/Private"

git -C "$repo_dir" diff --check

"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" -fsyntax-only "$consumer"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" -x c++ -fsyntax-only "$consumer"
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" -fsyntax-only "$compat_consumer"
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" -fsyntax-only "$gameplay_example"
stub_build_dir=$(mktemp -d)
trap 'rm -rf "$stub_build_dir"' EXIT HUP INT TERM
sanitizer_flags=
if [ "${UEC_SANITIZE:-0}" = 1 ]; then
    sanitizer_flags='-fsanitize=address,undefined -fno-omit-frame-pointer'
fi
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" \
    ${sanitizer_flags} "$consumer" "$host_stub" -o "$stub_build_dir/c_smoke"
"$stub_build_dir/c_smoke" >/dev/null
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" \
    ${sanitizer_flags} "$compat_consumer" "$host_stub" -o "$stub_build_dir/c_compat"
"$stub_build_dir/c_compat" >/dev/null
python3 -m json.tool "$repo_dir/UnrealCAPI.uplugin" >/dev/null
python3 -m json.tool "$repo_dir/UnrealCAPIHost.uproject" >/dev/null
sh -n "$repo_dir/tests/run_unreal_build.sh"
if [ ! -x "$repo_dir/tests/run_unreal_build.sh" ]; then
    printf '%s\n' 'The Unreal build gate must remain executable.' >&2
    exit 1
fi

if ! rg -q 'bEnableExceptions\s*=\s*false' "$repo_dir/Source/UnrealCAPI/UnrealCAPI.Build.cs"; then
    printf '%s\n' 'The Unreal module must keep C++ exceptions disabled at the ABI boundary.' >&2
    exit 1
fi

if [ ! -f "$repo_dir/IMPLEMENTATION_PLAN.md" ] || ! git -C "$repo_dir" check-ignore -q IMPLEMENTATION_PLAN.md; then
    printf '%s\n' 'IMPLEMENTATION_PLAN.md must exist and remain gitignored.' >&2
    exit 1
fi

for source_file in "$public_dir/uec_api.h" "$private_dir/uec_api.cpp" "$private_dir"/API/*.inl; do
    line_count=$(wc -l < "$source_file" | tr -d ' ')
    if [ "$line_count" -lt 400 ] || [ "$line_count" -gt 800 ]; then
        printf 'Private source file is outside the 400-800 line budget: %s (%s lines)\n' \
            "$source_file" "$line_count" >&2
        exit 1
    fi
done

printf '%s\n' 'C/C++ public-header, linked C consumer, Unreal descriptor, and private-layout checks passed.'
