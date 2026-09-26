#include "c_playable.h"

int uec_playable_header_consumer(void)
{
    uec_playable_sample_state state;
    state.done = UEC_FALSE;
    return state.done == UEC_FALSE ? 0 : 1;
}
