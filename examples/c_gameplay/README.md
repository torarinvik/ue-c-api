# C gameplay example

`c_gameplay.c` is a small host-side C consumer. It starts a game-thread flow
that obtains the default world, spawns the caller-selected actor class, moves
the actor three times from a looping timer callback, and releases the actor and
world handles when the sample is complete.

The caller owns the `uec_gameplay_example_state` storage and must keep it alive
until `done` becomes `UEC_TRUE`. The example is compiled by
`tests/run_checks.sh`; linking and running it requires loading the Unreal
plugin and obtaining the `uec_api` table through `uec_get_api`.
