# Safe consumer callback unload

The bridge cannot know when a consumer library is about to unload its callback
code. Use `uec_consumer_drain_gate` to close the race between stopping work and
observing a zero drain count. Initialize it before starting producer threads:

```c
static uec_consumer_drain_gate g_drain_gate =
    UEC_CONSUMER_DRAIN_GATE_INITIALIZER;

/* Before any callback-producing or queued-work API call: */
if (uec_consumer_drain_gate_try_begin(&g_drain_gate) == UEC_TRUE) {
    submit_work();
    (void)uec_consumer_drain_gate_end(&g_drain_gate);
}
```

Apply the gate to every path that can register a callback, subscription,
request, or queued job, including paths called from a callback. If admission is
denied, do not make that API call. The gate is consumer-local; a submission
that bypasses it is outside this unload guarantee.

For unload, close the gate and signal producer threads to stop. Confirm
`uec_consumer_drain_gate_is_quiescent` before canceling or unsubscribing owned
work, so no admitted submission can race with that cleanup. Then cancel and
unsubscribe on the game thread, and call `uec_consumer_drain_poll_gated` once
per tick. It reports drained only after the gate is closed and idle and the
bridge reports zero active subscriptions, pending requests, and in-flight
callbacks. Keep callback code and user data loaded while it reports false.
Before unloading, also ensure all producer threads have exited. Avoid blocking
the game thread joining a worker that may need game-thread callbacks to finish.
Once drained and producers have exited, release remaining bridge handles and
the context, destroy the gate, and then unload the consumer library. Live
handle counts in `out_stats` are diagnostic and do not delay callback unload
by themselves.

If a request cancellation reports that dispatch already dequeued the request,
the callback may still be running. Continue polling and retain its user data
until the in-flight callback count reaches zero.

The gate only protects calls routed through it. Its storage must remain alive
while producer threads can access it, and it may be destroyed only after the
gate is closed, every admitted operation has called `end`, and no thread can
access the gate again.
