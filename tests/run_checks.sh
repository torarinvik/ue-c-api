#!/usr/bin/env sh
set -eu

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
public_dir="$repo_dir/Source/UnrealCAPI/Public"
consumer="$repo_dir/tests/c_smoke/c_smoke.c"
gameplay_example="$repo_dir/examples/c_gameplay/c_gameplay.c"
private_dir="$repo_dir/Source/UnrealCAPI/Private"

"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" -fsyntax-only "$consumer"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" -x c++ -fsyntax-only "$consumer"
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" -fsyntax-only "$gameplay_example"
python3 -m json.tool "$repo_dir/UnrealCAPI.uplugin" >/dev/null
python3 -m json.tool "$repo_dir/UnrealCAPIHost.uproject" >/dev/null

for source_file in "$private_dir/uec_api.cpp" "$private_dir"/API/*.inl; do
    line_count=$(wc -l < "$source_file" | tr -d ' ')
    if [ "$line_count" -lt 400 ] || [ "$line_count" -gt 800 ]; then
        printf 'Private source file is outside the 400-800 line budget: %s (%s lines)\n' \
            "$source_file" "$line_count" >&2
        exit 1
    fi
done

printf '%s\n' 'C/C++ public-header, Unreal descriptor, and private-layout checks passed.'
