#!/usr/bin/env sh
set -eu

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
plugin_dir="$repo_dir/Plugins/UnrealCAPI"
public_dir="$plugin_dir/Source/UnrealCAPI/Public"
consumer="$repo_dir/tests/c_smoke/c_smoke.c"
layout_consumer="$repo_dir/tests/c_smoke/c_smoke_layout.c"
compat_consumer="$repo_dir/tests/c_smoke/c_compat.c"
host_stub="$repo_dir/tests/c_smoke/c_host_stub.c"
gameplay_example="$repo_dir/examples/c_gameplay/c_gameplay.c"
widget_ui_example="$repo_dir/examples/c_widget_ui/c_widget_ui.c"
widget_ui_example_dir="$repo_dir/examples/c_widget_ui"
widget_ui_smoke="$repo_dir/tests/c_smoke/c_widget_ui_smoke.c"
host_consumer="$repo_dir/Source/UnrealCAPIHost/Private/uec_host_smoke.c"
host_abi_consumer="$repo_dir/Source/UnrealCAPIHost/Private/Tests/uec_host_abi_smoke.c"
host_collision_consumer="$repo_dir/Source/UnrealCAPIHost/Private/Tests/uec_host_collision_smoke.c"
host_event_consumer="$repo_dir/Source/UnrealCAPIHost/Private/Tests/uec_host_event_bridge_smoke.c"
host_gameplay_consumer="$repo_dir/Source/UnrealCAPIHost/Private/Tests/uec_host_gameplay_example_smoke.c"
host_gameplay_translation_unit="$repo_dir/Source/UnrealCAPIHost/Private/uec_host_gameplay_example.c"
gameplay_header_consumer="$repo_dir/tests/c_smoke/c_gameplay_header.c"
gameplay_example_smoke="$repo_dir/tests/c_smoke/c_gameplay_example_smoke.c"
private_dir="$plugin_dir/Source/UnrealCAPI/Private"

git -C "$repo_dir" diff --check

"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" \
    -I "$widget_ui_example_dir" -fsyntax-only "$consumer"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" \
    -I "$widget_ui_example_dir" -x c++ -fsyntax-only "$consumer"
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" -fsyntax-only "$layout_consumer"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" -x c++ -fsyntax-only "$layout_consumer"
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" -fsyntax-only "$compat_consumer"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" -x c++ -fsyntax-only "$compat_consumer"
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" \
    -I "$repo_dir/examples/c_gameplay" -fsyntax-only "$gameplay_example"
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" \
    -I "$widget_ui_example_dir" -fsyntax-only "$widget_ui_example"
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" \
    -I "$widget_ui_example_dir" -fsyntax-only "$widget_ui_smoke"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" \
    -I "$widget_ui_example_dir" -x c++ -fsyntax-only "$widget_ui_example"
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" \
    -I "$repo_dir/examples/c_gameplay" -fsyntax-only "$gameplay_header_consumer"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" \
    -I "$repo_dir/examples/c_gameplay" -x c++ -fsyntax-only "$gameplay_header_consumer"
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" \
    -I "$repo_dir/examples/c_gameplay" -fsyntax-only "$gameplay_example_smoke"
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" -fsyntax-only "$host_consumer"
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" -fsyntax-only "$host_abi_consumer"
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" -fsyntax-only "$host_collision_consumer"
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" -fsyntax-only "$host_event_consumer"
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" \
    -I "$repo_dir/examples/c_gameplay" -fsyntax-only "$host_gameplay_consumer"
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" \
    -I "$repo_dir/examples/c_gameplay" -fsyntax-only "$host_gameplay_translation_unit"
stub_build_dir=$(mktemp -d)
trap 'rm -rf "$stub_build_dir"' EXIT HUP INT TERM
sanitizer_flags=
if [ "${UEC_SANITIZE:-0}" = 1 ]; then
    sanitizer_flags='-fsanitize=address,undefined -fno-omit-frame-pointer'
fi
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" \
    ${sanitizer_flags} -I "$repo_dir/examples/c_gameplay" -I "$widget_ui_example_dir" \
    "$consumer" "$layout_consumer" "$widget_ui_smoke" "$gameplay_example_smoke" \
    "$host_stub" "$host_consumer" "$host_abi_consumer" "$host_event_consumer" \
    "$gameplay_example" "$widget_ui_example" "$host_gameplay_consumer" \
    -o "$stub_build_dir/c_smoke"
"$stub_build_dir/c_smoke" >/dev/null
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" \
    ${sanitizer_flags} "$compat_consumer" "$host_stub" -o "$stub_build_dir/c_compat"
"$stub_build_dir/c_compat" >/dev/null
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" \
    ${sanitizer_flags} -c "$host_stub" -o "$stub_build_dir/c_host_stub.o"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" \
    ${sanitizer_flags} -x c++ -c "$compat_consumer" -o "$stub_build_dir/c_compat_cpp.o"
"${CXX:-c++}" ${sanitizer_flags} "$stub_build_dir/c_compat_cpp.o" \
    "$stub_build_dir/c_host_stub.o" -o "$stub_build_dir/c_compat_cpp"
"$stub_build_dir/c_compat_cpp" >/dev/null
python3 -m json.tool "$plugin_dir/UnrealCAPI.uplugin" >/dev/null
python3 -m json.tool "$repo_dir/UnrealCAPIHost.uproject" >/dev/null
for map_setting in EditorStartupMap GameDefaultMap ServerDefaultMap; do
    if ! rg -q "^$map_setting=/Engine/Maps/Templates/OpenWorld$" \
        "$repo_dir/Config/DefaultEngine.ini"; then
        printf 'The host project must configure %s.\n' "$map_setting" >&2
        exit 1
    fi
done
python3 "$repo_dir/tests/test_unreal_version.py" >/dev/null
python3 "$repo_dir/tests/test_unreal_runtime.py" >/dev/null
sh -n "$repo_dir/tests/run_unreal_build.sh"
if [ ! -x "$repo_dir/tests/run_unreal_build.sh" ]; then
    printf '%s\n' 'The Unreal build gate must remain executable.' >&2
    exit 1
fi

if ! rg -q 'bEnableExceptions\s*=\s*false' "$plugin_dir/Source/UnrealCAPI/UnrealCAPI.Build.cs"; then
    printf '%s\n' 'The Unreal module must keep C++ exceptions disabled at the ABI boundary.' >&2
    exit 1
fi
if ! rg -q 'UEC_BUILDING_LIBRARY' "$plugin_dir/Source/UnrealCAPI/UnrealCAPI.Build.cs"; then
    printf '%s\n' 'The Unreal module must define its C ABI export marker.' >&2
    exit 1
fi

if ! git -C "$repo_dir" check-ignore -q --no-index IMPLEMENTATION_PLAN.md; then
    printf '%s\n' 'IMPLEMENTATION_PLAN.md must remain gitignored when present locally.' >&2
    exit 1
fi

for source_file in "$public_dir/uec_api.h" "$private_dir/uec_api.cpp" \
    "$private_dir"/API/*.inl "$host_consumer"; do
    line_count=$(wc -l < "$source_file" | tr -d ' ')
    if [ "$line_count" -lt 400 ] || [ "$line_count" -gt 800 ]; then
        printf 'Implementation source unit is outside the 400-800 line budget: %s (%s lines)\n' \
            "$source_file" "$line_count" >&2
        exit 1
    fi
done

for source_file in "$repo_dir"/tests/c_smoke/c_host_stub_*.inl; do
    line_count=$(wc -l < "$source_file" | tr -d ' ')
    if [ "$line_count" -lt 400 ] || [ "$line_count" -gt 800 ]; then
        printf 'Smoke stub unit is outside the 400-800 line budget: %s (%s lines)\n' \
            "$source_file" "$line_count" >&2
        exit 1
    fi
done

for source_file in "$repo_dir"/tests/c_smoke/c_smoke.c \
    "$repo_dir"/tests/c_smoke/c_smoke_layout.c; do
    line_count=$(wc -l < "$source_file" | tr -d ' ')
    if [ "$line_count" -lt 400 ] || [ "$line_count" -gt 800 ]; then
        printf 'Smoke consumer/layout source unit is outside the 400-800 line budget: %s (%s lines)\n' \
            "$source_file" "$line_count" >&2
        exit 1
    fi
done

printf '%s\n' 'C/C++ public headers, linked current and legacy consumers, Unreal descriptor, and private-layout checks passed.'
