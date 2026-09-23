#!/usr/bin/env sh
set -eu

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
public_dir="$repo_dir/Plugins/UnrealCAPI/Source/UnrealCAPI/Public"
cc=${MINGW_CC:-x86_64-w64-mingw32-gcc}
cxx=${MINGW_CXX:-x86_64-w64-mingw32-g++}
objdump=${MINGW_OBJDUMP:-x86_64-w64-mingw32-objdump}
build_dir=$(mktemp -d "${TMPDIR:-/tmp}/uec-windows-abi.XXXXXX")

for tool in "$cc" "$cxx" "$objdump"; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        printf 'Required Windows ABI tool was not found: %s\n' "$tool" >&2
        exit 2
    fi
done

"$cc" -std=c11 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" \
    -fsyntax-only "$repo_dir/tests/c_smoke/c_smoke_layout.c"
"$cxx" -std=c++17 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" \
    -x c++ -fsyntax-only "$repo_dir/tests/c_smoke/c_smoke_layout.c"

"$cc" -std=c11 -Wall -Wextra -Werror -pedantic-errors -DUEC_BUILDING_LIBRARY \
    -I "$public_dir" -shared "$repo_dir/tests/c_smoke/c_host_stub.c" \
    -Wl,--out-implib,"$build_dir/libuec_host_stub.dll.a" \
    -o "$build_dir/uec_host_stub.dll"
"$cc" -std=c11 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" \
    -I "$repo_dir/examples/c_gameplay" -I "$repo_dir/examples/c_widget_ui" \
    "$repo_dir/tests/c_smoke/c_smoke.c" \
    "$repo_dir/tests/c_smoke/c_smoke_layout.c" \
    "$repo_dir/tests/c_smoke/c_widget_ui_smoke.c" \
    "$repo_dir/tests/c_smoke/c_gameplay_example_smoke.c" \
    "$repo_dir/Source/UnrealCAPIHost/Private/uec_host_smoke.c" \
    "$repo_dir/Source/UnrealCAPIHost/Private/Tests/uec_host_abi_smoke.c" \
    "$repo_dir/Source/UnrealCAPIHost/Private/Tests/uec_host_event_bridge_smoke.c" \
    "$repo_dir/Source/UnrealCAPIHost/Private/Tests/uec_host_gameplay_example_smoke.c" \
    "$repo_dir/examples/c_gameplay/c_gameplay.c" \
    "$repo_dir/examples/c_widget_ui/c_widget_ui.c" \
    -L"$build_dir" -luec_host_stub -o "$build_dir/c_smoke.exe"
"$cxx" -std=c++17 -Wall -Wextra -Werror -pedantic-errors -I "$public_dir" \
    -x c++ "$repo_dir/tests/c_smoke/c_compat.c" \
    -L"$build_dir" -luec_host_stub -o "$build_dir/c_compat_cpp.exe"

if ! "$objdump" -p "$build_dir/uec_host_stub.dll" | rg -q 'uec_get_api'; then
    printf '%s\n' 'The Windows host DLL did not export uec_get_api.' >&2
    exit 1
fi
for consumer in "$build_dir/c_smoke.exe" "$build_dir/c_compat_cpp.exe"; do
    if ! "$objdump" -p "$consumer" | rg -q 'DLL Name: uec_host_stub.dll'; then
        printf 'Windows consumer did not import the host DLL: %s\n' "$consumer" >&2
        exit 1
    fi
done

printf '%s\n' 'Windows x64 C/C++ ABI headers, DLL export, and current/legacy consumer links passed.'
