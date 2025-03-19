#include "fuse_virtio.h"

#include "vhost/fs.h"
#include "logging.h"
#include "../test_utils.h"

#include "fuse_kernel.h"

////////////////////////////////////////////////////////////////////////////////

struct fuse_virtio_dev
{
    struct vhd_fsdev_info fsdev;

    struct vhd_vdev* vdev;
    struct vhd_request_queue* rq;
};

struct fuse_virtio_queue
{
    // not used
};

struct fuse_virtio_request
{
    struct fuse_chan ch;
    struct vhd_bdev_io* bio;

    void* buffer;
    bool response_sent;

    struct {
        struct iovec* iov;
        size_t count;
    } in;

    struct {
        struct iovec* iov;
        size_t count;
    } out;

    struct iovec iov[1];
};

#define VIRTIO_REQ_FROM_CHAN(ch) containerof(ch, struct fuse_virtio_request, ch);

////////////////////////////////////////////////////////////////////////////////

static size_t iov_size(struct iovec* iov, size_t count)
{
    size_t len = 0;
    for (size_t i = 0; i < count; ++i) {
        len += iov[i].iov_len;
    }

    return len;
}

static void iov_copy_to_buffer(void* dst, struct iovec* src_iov, size_t src_count)
{
    while (src_count) {
        memcpy(dst, src_iov->iov_base, src_iov->iov_len);
        dst += src_iov->iov_len;

        src_iov++;
        src_count--;
    }
}

static void iov_copy_to_iov(
    struct iovec* dst_iov, size_t dst_count,
    struct iovec* src_iov, size_t src_count,
    size_t to_copy)
{
    size_t dst_offset = 0;
    /* Outer loop copies 'src' elements */
    while (to_copy) {
        VHD_ASSERT(src_count);
        size_t src_len = src_iov[0].iov_len;
        size_t src_offset = 0;

        if (src_len > to_copy) {
            src_len = to_copy;
        }
        /* Inner loop copies contents of one 'src' to maybe multiple dst. */
        while (src_len) {
            VHD_ASSERT(dst_count);
            size_t dst_len = dst_iov[0].iov_len - dst_offset;
            if (dst_len > src_len) {
                dst_len = src_len;
            }

            memcpy(dst_iov[0].iov_base + dst_offset,
                   src_iov[0].iov_base + src_offset, dst_len);
            src_len -= dst_len;
            to_copy -= dst_len;
            src_offset += dst_len;
            dst_offset += dst_len;

            VHD_ASSERT(dst_offset <= dst_iov[0].iov_len);
            if (dst_offset == dst_iov[0].iov_len) {
                dst_offset = 0;
                dst_iov++;
                dst_count--;
            }
        }
        src_iov++;
        src_count--;
    }
}

static void split_request_buffers(
    struct fuse_virtio_request* req,
    struct vhd_sglist* sglist)
{
    size_t i = 0;

    req->in.iov = req->iov;
    for (; i < sglist->nbuffers; ++i) {
        struct vhd_buffer* buf = &sglist->buffers[i];
        if (buf->write_only) {
            break;
        }
        struct iovec* iov = &req->in.iov[req->in.count++];
        iov->iov_base = buf->base;
        iov->iov_len = buf->len;
    }

    req->out.iov = &req->iov[i];
    for (; i < sglist->nbuffers; ++i) {
        struct vhd_buffer* buf = &sglist->buffers[i];
        if (!buf->write_only) {
            break;
        }
        struct iovec* iov = &req->out.iov[req->out.count++];
        iov->iov_base = buf->base;
        iov->iov_len = buf->len;
    }
}

static void complete_request(struct fuse_virtio_request* req, int res)
{
    VHD_ASSERT(!req->response_sent);
    req->response_sent = true;

    vhd_complete_bio(req->bio, res);

    vhd_free(req->buffer);
    vhd_free(req);
}

static bool is_write_request(struct fuse_in_header* in)
{
    return in->opcode == FUSE_WRITE;
}

static bool is_oneway_request(struct fuse_in_header* in)
{
    return in->opcode == FUSE_FORGET || in->opcode == FUSE_BATCH_FORGET;
}

static int process_write_request(
    struct fuse_session* se,
    struct fuse_virtio_request* req)
{
    size_t len = iov_size(req->in.iov, req->in.count);
    if (len > se->bufsize) {
        return -EINVAL;
    }

    /*
     * fuse_session_process_buf_int strictly expects fuse_in_header followed by
     * opcode-specific header in the first segment, and the rest either fully
     * contained in that very same (and only) segment, or be in the following
     * segments.
     */
    static const size_t buf0len =
        sizeof(struct fuse_in_header) + sizeof(struct fuse_write_in);
    if (len <= buf0len) {
        return -EINVAL;
    }

    size_t count, hdr_buf, iov_idx, iov_off;
    if (req->in.count == 1 || req->in.iov[0].iov_len == buf0len) {
        /* original framing matches expected, pass through */
        count = req->in.count;
        hdr_buf = 0;
        iov_idx = 0;
        iov_off = 0;
    } else if (req->in.iov[0].iov_len > buf0len) {
        /* first segment contains headers and data; split the two apart */
        count = req->in.count + 1;
        hdr_buf = 1;
        iov_idx = 0;
        iov_off = buf0len;
    } else {
        /* need intermediate buffer for headers */
        size_t left;
        void *ptr;
        req->buffer = vhd_alloc(len);
        for (ptr = req->buffer, iov_idx = 0, left = buf0len; ; iov_idx++) {
            iov_off = MIN(left, req->in.iov[iov_idx].iov_len);
            memcpy(ptr, req->in.iov[iov_idx].iov_base, iov_off);
            ptr += iov_off;
            left -= iov_off;
            /* break *before* iov_idx increment */
            if (left == 0) {
                break;
            }
        }
        if (iov_off == req->in.iov[iov_idx].iov_len) {
            /* full segment -- skip to next one */
            iov_idx++;
            iov_off = 0;
        }
        count = req->in.count - iov_idx + 1;
        hdr_buf = 1;
    }

    struct fuse_bufvec* bufv = vhd_zalloc(
        sizeof(struct fuse_bufvec) +
        sizeof(struct fuse_buf) * (count - 1));

    bufv->count = count;

    if (hdr_buf) {
        bufv->buf[0].mem = req->buffer ? req->buffer : req->in.iov[0].iov_base;
        bufv->buf[0].size = buf0len;
        bufv->buf[0].fd = -1;
    }

    size_t idx;
    for (idx = hdr_buf; idx < count; idx++, iov_idx++) {
        bufv->buf[idx].mem = req->in.iov[iov_idx].iov_base + iov_off;
        bufv->buf[idx].size = req->in.iov[iov_idx].iov_len - iov_off;
        iov_off = 0;
        bufv->buf[idx].fd = -1;
    }

    fuse_session_process_buf_int(se, bufv, &req->ch);

    vhd_free(bufv);
    return 0;
}

static int process_generic_request(
    struct fuse_session* se,
    struct fuse_virtio_request* req)
{
    size_t len = iov_size(req->in.iov, req->in.count);
    if (len > se->bufsize) {
        return -EINVAL;
    }

    req->buffer = vhd_alloc(len);
    iov_copy_to_buffer(req->buffer, req->in.iov, req->in.count);

    struct fuse_bufvec bufv = FUSE_BUFVEC_INIT(len);
    bufv.buf[0].mem = req->buffer;

    fuse_session_process_buf_int(se, &bufv, &req->ch);

    return 0;
}

static int process_request(struct fuse_session* se, struct vhd_bdev_io* bio)
{
    VHD_ASSERT(bio->sglist.nbuffers > 0);

    struct fuse_virtio_request* req = vhd_zalloc(
        sizeof(struct fuse_virtio_request) +
        sizeof(struct iovec) * (bio->sglist.nbuffers - 1));

    req->bio = bio;

    split_request_buffers(req, &bio->sglist);
    VHD_ASSERT(req->in.count + req->out.count == bio->sglist.nbuffers);

    VHD_LOG_DEBUG("request with %zu IN desc of length %zu "
                  "and %zu OUT desc of length %zu\n",
        req->in.count, iov_size(req->in.iov, req->in.count),
        req->out.count, iov_size(req->out.iov, req->out.count));

    VHD_ASSERT(req->in.count >= 1);
    VHD_ASSERT(req->in.iov[0].iov_len >= sizeof(struct fuse_in_header));

    struct fuse_in_header* in = (struct fuse_in_header*) req->in.iov[0].iov_base;

    // We could not trust client so we will copy request to a safe place.
    // But for WRITE request we do not want to copy payload - just headers!
    int res = is_write_request(in)
        ? process_write_request(se, req)
        : process_generic_request(se, req);

    // For now there is no way to notify one-way requests completion
    if (res < 0 || is_oneway_request(in)) {
        complete_request(req, res);
    }

    return res;
}

static void unregister_complete(void* ctx)
{
    struct fuse_session* se = ctx;
    struct fuse_virtio_dev* dev = se->virtio_dev;

    VHD_LOG_INFO("stopping device %s", dev->fsdev.socket_path);
    vhd_stop_queue(dev->rq);
}

static void unregister_complete_and_free_dev(void* ctx)
{
    unregister_complete(ctx);

    struct fuse_session* se = ctx;
    struct fuse_virtio_dev* dev = se->virtio_dev;

    vhd_release_request_queue(dev->rq);
    vhd_free(dev);
}

////////////////////////////////////////////////////////////////////////////////

#if 0
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
#endif

int virtio_session_mount(struct fuse_session* se)
{
    struct fuse_virtio_dev* dev = vhd_zalloc(sizeof(struct fuse_virtio_dev));

    // no need to supply tag here - it will be handled by the QEMU
    dev->fsdev.socket_path = se->vu_socket_path;
    dev->fsdev.num_queues = se->thread_pool_size;

    VHD_LOG_INFO("starting device %s", dev->fsdev.socket_path);

    // TODO: multiple request queues
    dev->rq = vhd_create_request_queue();
    if (!dev->rq) {
        vhd_free(dev);
        return -ENOMEM;
    }

    int ret;
    if ((ret = vhd_start_vhost_server(vhd_log_stderr)) < 0) {
        return ret;
    }

    dev->vdev = vhd_register_fs(&dev->fsdev, dev->rq, NULL);
    if (!dev->vdev) {
        vhd_release_request_queue(dev->rq);
        vhd_free(dev);
        return -ENOMEM;
    }

    int err = chmod(se->vu_socket_path, S_IRGRP | S_IWGRP | S_IRUSR | S_IWUSR);
    if (err < 0) {
        vhd_unregister_fs(dev->vdev, unregister_complete_and_free_dev, se);
        return err;
    }

    se->virtio_dev = dev;

    vhd_log_stderr(LOG_INFO, "Virtiofs test server started");

    return 0;
}

void virtio_session_close(struct fuse_session* se)
{
    struct fuse_virtio_dev* dev = se->virtio_dev;

    VHD_LOG_INFO("destroying device %s", dev->fsdev.socket_path);
    vhd_release_request_queue(dev->rq);
    vhd_free(dev);
}

void virtio_session_exit(struct fuse_session* se)
{
    struct fuse_virtio_dev* dev = se->virtio_dev;

    VHD_LOG_INFO("unregister device %s", dev->fsdev.socket_path);
    vhd_unregister_fs(dev->vdev, unregister_complete, se);
}

int virtio_session_loop(struct fuse_session* se)
{
    struct fuse_virtio_dev* dev = se->virtio_dev;

    int res;
    for (;;) {
        res = vhd_run_queue(dev->rq);
        if (res != -EAGAIN) {
            if (res < 0) {
                VHD_LOG_WARN("request queue failure %d", -res);
            }
            break;
        }

        struct vhd_request req;
        while (vhd_dequeue_request(dev->rq, &req)) {
            res = process_request(se, req.bio);
            if (res < 0) {
                VHD_LOG_WARN("request processing failure %d", -res);
            }
        }
    }

    se->exited = 1;
    return res;
}

int virtio_send_msg(
    struct fuse_session* se,
    struct fuse_chan* ch,
    struct iovec* iov,
    int count)
{
    struct fuse_virtio_request* req = VIRTIO_REQ_FROM_CHAN(ch);

    VHD_ASSERT(count >= 1);
    VHD_ASSERT(iov[0].iov_len >= sizeof(struct fuse_out_header));

    size_t response_bytes = iov_size(iov, count);
    VHD_LOG_DEBUG("response with %d desc of length %zu\n",
        count, response_bytes);

    size_t out_bytes = iov_size(req->out.iov, req->out.count);
    if (out_bytes < response_bytes) {
        VHD_LOG_ERROR("request buffers too small for response - "
                      "requested:%zu, available:%zu\n",
            response_bytes, out_bytes);
        return -E2BIG;
    }

    iov_copy_to_iov(req->out.iov, req->out.count, iov, count, response_bytes);

    complete_request(req, 0);
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
    // TODO
    return -1;
}
