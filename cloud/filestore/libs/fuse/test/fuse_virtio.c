#include "fuse_virtio.h"

#include <cloud/filestore/libs/fuse/fuse.h>

#include <contrib/libs/virtiofsd/fuse.h>
#include <contrib/libs/virtiofsd/fuse_i.h>
#include <contrib/libs/virtiofsd/fuse_log.h>
#include <contrib/libs/virtiofsd/fuse_lowlevel.h>
#include <contrib/libs/virtiofsd/fuse_virtio.h>

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////

#if !defined(containerof)
#define containerof(ptr, type, member)                                         \
({                                                                             \
    const typeof( ((type*)0)->member ) * __mptr = ((void*)(ptr));              \
    (type*)( (char*)__mptr - offsetof(type, member) );                         \
})
#endif

////////////////////////////////////////////////////////////////////////////////

struct list_node
{
    struct list_node* next;
    struct list_node* prev;
};

struct fuse_virtio_dev
{
    // not used
};

struct fuse_virtio_queue
{
    // not used
};

struct fuse_virtio_request
{
    struct fuse_chan ch;
    struct list_node node;

    const void* in;
    size_t in_size;

    void* out;
    size_t out_size;

    virtio_request_cb cb;
    void* context;
};

#define VIRTIO_REQ_FROM_CHAN(ptr) containerof(ptr, struct fuse_virtio_request, ch);
#define VIRTIO_REQ_FROM_NODE(ptr) containerof(ptr, struct fuse_virtio_request, node);

////////////////////////////////////////////////////////////////////////////////

// TODO: move to fuse_virtio_dev
struct list_node g_requests;
pthread_mutex_t g_requests_lock;

////////////////////////////////////////////////////////////////////////////////

static void fuse_session_exit_safe(struct fuse_session* se)
{
    __atomic_store_n(&se->exited, 1, __ATOMIC_RELEASE);
}

static int fuse_session_exited_safe(struct fuse_session* se)
{
    return __atomic_load_n(&se->exited, __ATOMIC_RELAXED);
}

static void list_init_node(struct list_node* node)
{
    node->next = node;
    node->prev = node;
}

static void list_del_node(struct list_node* node)
{
    struct list_node* prev = node->prev;
    struct list_node* next = node->next;
    prev->next = next;
    next->prev = prev;
}

static void list_push_node(struct list_node* node, struct list_node* next)
{
    struct list_node* prev = node->prev;
    node->prev = next;
    next->next = node;
    next->prev = prev;
    prev->next = next;
}

static struct list_node* list_pop_node(struct list_node* node)
{
    struct list_node* next = node->next;
    if (next != node) {
        list_del_node(next);
        list_init_node(next);
        return next;
    }

    return NULL;
}

static size_t iov_size(const struct iovec* iov, size_t count)
{
    size_t len = 0;
    for (size_t i = 0; i < count; ++i) {
        len += iov[i].iov_len;
    }

    return len;
}

static void iov_copy_to_buffer(void* dst, const struct iovec* src_iov, size_t src_count)
{
    while (src_count) {
        memcpy(dst, src_iov->iov_base, src_iov->iov_len);
        dst += src_iov->iov_len;

        src_iov++;
        src_count--;
    }
}

static uint64_t request_unique(const struct fuse_virtio_request* req)
{
    assert(req->in_size >= sizeof(struct fuse_in_header));
    return ((struct fuse_in_header *)req->in)->unique;
}

static bool is_oneway_request(const struct fuse_in_header* in)
{
    return in->opcode == FUSE_FORGET || in->opcode == FUSE_BATCH_FORGET;
}

////////////////////////////////////////////////////////////////////////////////

uint64_t fuse_req_unique(fuse_req_t req)
{
    return req->unique;
}

void fuse_session_setparams(
    struct fuse_session* se,
    const struct fuse_session_params* params)
{
    se->conn.proto_major = params->proto_major;
    se->conn.proto_minor = params->proto_minor;
    se->conn.capable = params->capable;
    se->conn.want = params->want;
    se->bufsize = params->bufsize;

    se->got_init = 1;
    se->got_destroy = 0;
}

void fuse_session_getparams(
    struct fuse_session* se,
    struct fuse_session_params* params)
{
    params->proto_major = se->conn.proto_major;
    params->proto_minor = se->conn.proto_minor;
    params->capable = se->conn.capable;
    params->want = se->conn.want;
    params->bufsize = se->bufsize;
}

int virtio_server_init()
{
    list_init_node(&g_requests);
    pthread_mutex_init(&g_requests_lock, NULL);
    return 0;
}

void virtio_server_term()
{
    pthread_mutex_lock(&g_requests_lock);

    struct list_node* node;
    while ((node = list_pop_node(&g_requests)) != NULL) {
        struct fuse_virtio_request* req = VIRTIO_REQ_FROM_NODE(node);
        req->cb(req->context, -1);
    }

    pthread_mutex_unlock(&g_requests_lock);
    pthread_mutex_destroy(&g_requests_lock);
}

int virtio_session_mount(struct fuse_session* se)
{
    se->virtio_dev = (struct fuse_virtio_dev*)1;
    return 0;
}

void virtio_session_close(struct fuse_session* se)
{
    (void)se;
}

void virtio_session_exit(struct fuse_session* se)
{
    fuse_session_exit_safe(se);
}

void virtio_session_enqueue(
    struct fuse_session* se,
    const void* in,
    size_t in_size,
    void* out,
    size_t out_size,
    virtio_request_cb cb,
    void* context)
{
    (void)se;

    struct fuse_virtio_request* req = calloc(1, sizeof(struct fuse_virtio_request));
    list_init_node(&req->node);
    req->in = in;
    req->in_size = in_size;
    req->out = out;
    req->out_size = out_size;
    req->cb = cb;
    req->context = context;

    fuse_log(FUSE_LOG_DEBUG, "[enqueue] unique:%lu, insize:%zu, outsize:%zu",
        request_unique(req), req->in_size, req->out_size);

    pthread_mutex_lock(&g_requests_lock);
    list_push_node(&g_requests, &req->node);
    pthread_mutex_unlock(&g_requests_lock);
}

int virtio_session_loop(struct fuse_session* se)
{
    struct list_node* node;
    while (!fuse_session_exited_safe(se)) {
        pthread_mutex_lock(&g_requests_lock);
        node = list_pop_node(&g_requests);
        pthread_mutex_unlock(&g_requests_lock);

        if (node) {
            struct fuse_virtio_request* req = VIRTIO_REQ_FROM_NODE(node);
            fuse_log(FUSE_LOG_DEBUG, "[process] unique:%lu, insize:%zu, outsize:%zu",
                request_unique(req), req->in_size, req->out_size);

            struct fuse_bufvec bufv = FUSE_BUFVEC_INIT(req->in_size);
            bufv.buf[0].mem = (void*)req->in;

            const bool oneway = is_oneway_request(req->in);

            fuse_session_process_buf_int(se, &bufv, &req->ch);

            if (oneway) {
                req->cb(req->context, 0);
                free(req);
            }
        }
    }

    return 0;
}

int virtio_send_msg(
    struct fuse_session* se,
    struct fuse_chan* ch,
    struct iovec* iov,
    int count)
{
    struct fuse_virtio_request* req = VIRTIO_REQ_FROM_CHAN(ch);

    if (fuse_session_exited_safe(se)) {
        fuse_log(FUSE_LOG_DEBUG, "[send_msg] unique: %lu, reject send to exited session",
            request_unique(req));

        struct fuse_out_header *out = iov[0].iov_base;
        out->error = EPIPE;
    }

    size_t response_bytes = iov_size(iov, count);
    fuse_log(FUSE_LOG_DEBUG, "[send_msg] response bytes %u, out bytes %u",
        response_bytes, req->out_size);

    if (response_bytes > req->out_size) {
        return -E2BIG;
    }

    iov_copy_to_buffer(req->out, iov, count);
    req->cb(req->context, 0);

    free(req);
    return 0;
}

int virtio_send_data_iov(
    struct fuse_session* se,
    struct fuse_chan* ch,
    struct iovec* iov,
    int count,
    struct fuse_bufvec* buf,
    size_t len)
{
    (void)se;
    (void)ch;
    (void)iov;
    (void)count;
    (void)buf;
    (void)len;
    return 0;
}

int fuse_cancel_request(
    fuse_req_t req,
    enum fuse_cancelation_code code)
{
    (void)code;

    fuse_log(FUSE_LOG_DEBUG, "[cancel_request] unique: %lu, complete request with EINTR code",
        fuse_req_unique(req));

    struct fuse_out_header out = {
        .unique = req->unique,
        .error = EINTR,
    };

    struct iovec iov;
    iov.iov_base = &out;
    iov.iov_len = sizeof(out);

    struct fuse_chan* ch = req->ch;
    struct fuse_virtio_request* vreq = VIRTIO_REQ_FROM_CHAN(ch);

    iov_copy_to_buffer(vreq->out, &iov, 1);

    vreq->cb(vreq->context, 0);

    free(vreq);
    free(req);
    return 0;
}
