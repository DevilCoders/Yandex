#include <library/cpp/deprecated/atomic/atomic.h>

#ifdef _linux_

#include <contrib/libs/libaio/libaio.h>

namespace NDoom {

struct aio_ring {
    unsigned id;         /** kernel internal index number */
    unsigned nr;         /** number of io_events */
    unsigned head;
    unsigned tail;

    unsigned magic;
    unsigned compat_features;
    unsigned incompat_features;
    unsigned header_length; /** size of aio_ring */

    struct io_event events[0];
};

#define AIO_RING_MAGIC  0xa10a10a1
int user_io_getevents(io_context_t aio_ctx, unsigned int max, struct io_event *events) {
    long i = 0;
    unsigned head;
    struct aio_ring *ring = (struct aio_ring*) aio_ctx;

    if (ring == nullptr || ring->magic != AIO_RING_MAGIC) {
        return -1;
    }

    while (i < max) {
        head = ring->head;

        if (head == ring->tail) {
            break;
        } else {
            events[i] = ring->events[head];
            ATOMIC_COMPILER_BARRIER();
            ring->head = (head + 1) % ring->nr;
            i++;
        }
    }

    return i;
}

} //namespace NDoom

#endif
