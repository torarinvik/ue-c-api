#!/usr/bin/env sh
set -eu

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
public_dir="$repo_dir/Source/UnrealCAPI/Public"
consumer="$repo_dir/tests/c_smoke/c_smoke.c"
gameplay_example="$repo_dir/examples/c_gameplay/c_gameplay.c"

"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" -fsyntax-only "$consumer"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" -x c++ -fsyntax-only "$consumer"
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" -fsyntax-only "$gameplay_example"
python3 -m json.tool "$repo_dir/UnrealCAPI.uplugin" >/dev/null
python3 -m json.tool "$repo_dir/UnrealCAPIHost.uproject" >/dev/null

printf '%s\n' 'C/C++ public-header and Unreal descriptor checks passed.'
