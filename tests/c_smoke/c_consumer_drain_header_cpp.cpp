#include "c_consumer_drain.h"

static_assert(sizeof(uec_consumer_drain_gate) == sizeof(void*),
              "the C++ helper header must keep opaque gate storage");

int main()
{
    uec_consumer_drain_gate gate = UEC_CONSUMER_DRAIN_GATE_INITIALIZER;
    return gate.state == nullptr ? 0 : 1;
}
