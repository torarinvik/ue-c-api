# Safe consumer callback unload

The bridge cannot know when a consumer library is about to unload its callback
code. The consumer must first stop scheduling new work, unsubscribe and cancel
every operation it owns, and keep callback functions plus their user data
loaded while accepted work drains.

Call `uec_consumer_drain_poll` on the game thread once per tick. It reports
`out_drained == UEC_TRUE` only after the bridge reports zero active
subscriptions, pending requests, and in-flight callbacks. Keep the API table,
context, consumer state, and callback code alive while the result is false.
After it becomes true, release the consumer's remaining bridge handles and
context before unloading the library. Live handle counts are returned in
`out_stats` for diagnostics; they do not delay callback-code unload by
themselves.

If a request cancellation reports that dispatch already dequeued the request,
the callback may still be running. Continue polling and retain its user data
until the in-flight callback count reaches zero.
