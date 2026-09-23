# C gameplay example

`c_gameplay.h` declares the caller-owned state and start/cancel functions;
`c_gameplay.c` implements a small host-side C consumer. It obtains the default
world, spawns the caller-selected actor class, moves the actor three times from
a looping timer, emits and validates a synchronous event-bridge callback on
each tick, and releases its actor and world handles when complete.

The caller keeps the `uec_gameplay_example_state` and API context alive until
`done` becomes `UEC_TRUE`, then releases the context. Start, cancel, and timer
callbacks run on the game thread. The example uses the same C source in the
packaged host smoke through a thin C translation-unit wrapper, so contributors
can see it compile and run against Unreal as well as pass the portable
`tests/run_checks.sh` gate.

Before reading function pointers, the example checks that the API table reaches
the `emit_actor_event_bridge` entry and verifies every operation it uses. A
short table or missing function returns `UEC_RESULT_UNSUPPORTED` and leaves the
state marked done without starting any work.
