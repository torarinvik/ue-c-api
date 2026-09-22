# C ABI smoke consumers

This is an engine-independent compile check for the public header. From the repository root, run the complete local checks with:

```sh
sh tests/run_checks.sh
```

To run only the C11 syntax check:

```sh
cc -std=c11 -I../../Source/UnrealCAPI/Public -fsyntax-only c_smoke.c
```

The local gate links the runtime consumer and its layout assertions against
`tests/c_smoke/c_host_stub.c` so bootstrap, table calls, output clearing, and
the append-only prefix can run without Unreal. The stub is assembled from the
focused `c_host_stub_bootstrap.inl` and `c_host_stub_reflection.inl` units.
`uec_get_api` is still exported by the Unreal plugin when it is loaded into a
real host. `c_compat.c` models an older minor-version consumer that only uses
the stable table prefix; a linked Unreal-host compatibility run remains part of
the engine-dependent release gate.
