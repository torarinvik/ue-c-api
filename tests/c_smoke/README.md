# C ABI smoke consumer

This is an engine-independent compile check for the public header. From the repository root, run the complete local checks with:

```sh
sh tests/run_checks.sh
```

To run only the C11 syntax check:

```sh
cc -std=c11 -I../../Source/UnrealCAPI/Public -fsyntax-only c_smoke.c
```

The executable is intentionally not linked here: `uec_get_api` is exported by the Unreal plugin when it is loaded into an Unreal host. The program documents the expected bootstrap and function-table usage for a real C consumer.
