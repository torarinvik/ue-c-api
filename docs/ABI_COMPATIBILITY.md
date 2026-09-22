# ABI compatibility and migration

The public ABI is exposed through the `uec_api` function table returned by
`uec_get_api`. The table is append-only within a major version. A bridge may
add new fields at the end of the table and increase `UEC_ABI_MINOR`; it must
not reorder, remove, or change the types of existing fields.

Consumers request the newest minor version they understand. A bridge accepts
that request when the requested major matches and the requested minor is no
greater than the bridge's minor. Consumers may request an older minor and use
the compatible prefix. Before calling an optional entry, compare its offset
and size with `api->struct_size`; older bridges can return a shorter table.
The `abi_major`, `abi_minor`, and `struct_size` fields describe the table that
was returned, so consumers should use those values rather than assuming the
header and bridge were built together.

Major-version changes are reserved for incompatible changes such as changing
the meaning or type of an existing field, changing a public POD layout, or
removing a function. A new major version gets a new compatibility prefix and
must keep a separate migration note. Minor releases add optional operations,
capability bits, result codes, or fields at the end of an existing structure.
Existing result-code numeric values and enum values remain stable. New enum
values are handled as unknown by older consumers.

When a size-tagged structure grows, the new bridge must continue accepting the
previous structure prefix. It may read an appended input field or write an
appended output field only when `struct_size` reaches that field; otherwise it
must preserve the prior behavior for the older prefix.

Every new function must document its valid thread, handle ownership, callback
thread, cancellation behavior, output-clearing behavior, and unsupported
engine contexts. New callbacks borrow `user_data` unless the function
explicitly says otherwise. A returned object or handle is owned by the caller
until it is released or the documented callback lifetime ends.

The public header contains only C-compatible declarations. Consumers should
compile it as C11 or later and may use it from C++. Private Unreal headers and
the plugin's C++ implementation are not ABI dependencies for consumer code.
The checked-in C smoke fixtures exercise the current table and a separately
compiled ABI 1.135 header against the ABI 1.140 bridge, in both C and C++ modes;
the older consumer calls the stable table prefix and the ABI 1.135 data methods.
The same checks must pass before a release is tagged.

## Deprecation process

An operation is deprecated by documentation first. The changelog identifies
the replacement and the first minor version containing the deprecation note.
Deprecated fields remain callable for the rest of the major version unless
the engine makes their behavior unsafe. Removal or incompatible semantic
changes require a new major version. Capability bits describe optional
runtime support; a missing bit or `UEC_RESULT_UNSUPPORTED` is the supported
way to handle an operation that the loaded bridge cannot provide.

## Engine compatibility

The C ABI does not make Unreal C++ binaries portable across engine releases.
Build the plugin against the exact Unreal Engine patch used by the host. The
repository currently targets UE 5.8.x and records the selected patch in
`docs/BUILD_MATRIX.md`. When a newer UE release becomes the supported target,
update the descriptor, build matrix, README, and compatibility notes together,
then rerun the portable gate and the full engine verification matrix.

## Release checklist

- Update `UEC_ABI_MINOR` only for an append-only compatible extension.
- Append function-table fields and enum values; never reorder existing fields or
  change published enum values.
- Add a capability bit when runtime availability is optional.
- Add C11/C++17 syntax and layout assertions for new public data.
- Extend the current and old-minor linked smoke fixtures where applicable.
- Record the change in `CHANGELOG.md` and document ownership and threading.
- Build and exercise the plugin in the supported UE editor and packaged modes.
