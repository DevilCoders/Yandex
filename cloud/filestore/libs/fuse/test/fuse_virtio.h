#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct fuse_session;

typedef void (* virtio_request_cb)(void* context, int result);

int virtio_server_init();
void virtio_server_term();

void virtio_session_enqueue(
    struct fuse_session* se,
    // input buffer
    const void* in,
    size_t in_size,
    // output buffer
    void* out,
    size_t out_size,
    // completion callback
    virtio_request_cb cb,
    void* context);

#ifdef __cplusplus
}
#endif
