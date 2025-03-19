#define _GNU_SOURCE 1

#include <libaio.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <inttypes.h>
#include <signal.h>

#include "vhost/server.h"
#include "vhost/blockdev.h"
#include "test_utils.h"
#include "platform.h"

#define MAX_AIO_QUEUE_LEN 32
#define MAX_AIO_EVENTS 32

#define DIE(fmt, ...)                              \
do {                                               \
    vhd_log_stderr(LOG_ERROR, fmt, ##__VA_ARGS__); \
    exit(EXIT_FAILURE);                            \
} while (0)

#define PERROR(fun, err)                                     \
do {                                                         \
    vhd_log_stderr(LOG_ERROR, "%s: %s", fun, strerror(err)); \
} while (0)

/*
 * Configuration used for backend initialization.
 */
struct backend_config {
    const char *socket_path;
    const char *serial;
    const char *blk_file;
    unsigned long delay;
    bool readonly;
};

/*
 * File block backend.
 */
struct backend {
    struct vhd_vdev *handler;
    struct vhd_bdev_info info;
    unsigned long delay;
    int fd;
    io_context_t io_ctx;
};

/*
 * Structure per vhost eventloop, contains link to backend.
 */
struct queue {
    struct vhd_request_queue *rq;
    struct backend *bdev;
};

/*
 * Single IO request. Also map libvhost's vhd_buffer to iovec.
 */
struct request {
    struct vhd_bdev_io *bio;
    struct iocb ios;
    bool bounce_buf;
    struct iovec iov[]; /* bio->sglist.nbuffers */
};

/*
 * Allocate and prepare IO request (fill iovecs and AIO control block).
 */
static struct iocb *prepare_io_operation(struct vhd_request *lib_req)
{
    struct backend *bdev = vhd_vdev_get_priv(lib_req->vdev);
    struct vhd_bdev_io *bio = lib_req->bio;

    uint64_t offset = bio->first_sector * VHD_SECTOR_SIZE;
    struct vhd_buffer *buffers = bio->sglist.buffers;
    unsigned nbufs = bio->sglist.nbuffers;

    struct request *req = calloc(1,
        sizeof(struct request) + sizeof(struct iovec) * bio->sglist.nbuffers);

    vhd_log_stderr(LOG_DEBUG,
        "%s request, %u parts: start block %" PRIu64 ", blocks count %" PRIu64,
        bio->type == VHD_BDEV_READ ? "Read" : "Write",
        bio->sglist.nbuffers,
        bio->first_sector,
        bio->total_sectors);

    req->bio = bio;

    for (uint32_t i = 0; i < nbufs; i++) {
        /*
         * Windows allows i/o with buffers not aligned to i/o block size, but
         * Linux doesn't, so use bounce buffer in this case.
         * Note: the required alignment is the logical block size of the
         * underlying storage; assume it to equal the sector size as BIOS
         * requires sector-granular i/o anyway.
         */
        if (!VHD_IS_ALIGNED((uintptr_t)buffers[i].base, VHD_SECTOR_SIZE) ||
            !VHD_IS_ALIGNED(buffers[i].len, VHD_SECTOR_SIZE)) {
            req->bounce_buf = true;
            req->iov[0].iov_len = bio->total_sectors * VHD_SECTOR_SIZE;
            req->iov[0].iov_base = aligned_alloc(VHD_SECTOR_SIZE,
                                                 req->iov[0].iov_len);

            if (bio->type == VHD_BDEV_WRITE) {
                void *ptr = req->iov[0].iov_base;
                for (i = 0; i < nbufs; i++) {
                    memcpy(ptr, buffers[i].base, buffers[i].len);
                    ptr += buffers[i].len;
                }
            }

            nbufs = 1;

            break;
        }

        req->iov[i].iov_base = buffers[i].base;
        req->iov[i].iov_len = buffers[i].len;
    }

    if (bio->type == VHD_BDEV_READ) {
        io_prep_preadv(&req->ios, bdev->fd, req->iov, nbufs, offset);
    } else {
        io_prep_pwritev(&req->ios, bdev->fd, req->iov, nbufs, offset);
    }

    /* set AFTER io_prep_* because it fill all fields with zeros */
    req->ios.data = req;
    vhd_log_stderr(LOG_DEBUG, "Prepared IO request with addr: %p", req);

    return &req->ios;
}

static void complete_request(struct request *req,
                             enum vhd_bdev_io_result status)
{
    if (req->bounce_buf && req->bio->type == VHD_BDEV_READ) {
        if (status == VHD_BDEV_SUCCESS) {
            struct vhd_buffer *buffers = req->bio->sglist.buffers;
            unsigned nbufs = req->bio->sglist.nbuffers, i;
            void *ptr = req->iov[0].iov_base;
            for (i = 0; i < nbufs; i++) {
                memcpy(buffers[i].base, ptr, buffers[i].len);
                ptr += buffers[i].len;
            }
        }
        free(req->iov[0].iov_base);
    }
    vhd_complete_bio(req->bio, status);
}

/*
 * IO requests handler thread, that serve all requests in one vhost eventloop.
 */
static void *io_handle(void *opaque)
{
    struct queue *qdev = (struct queue *) opaque;

    while (true) {
        struct vhd_request req;

        int ret = vhd_run_queue(qdev->rq);
        if (ret != -EAGAIN) {
            if (ret < 0) {
                vhd_log_stderr(LOG_ERROR, "vhd_run_queue error: %d", ret);
            }
            break;
        }

        while (vhd_dequeue_request(qdev->rq, &req)) {
            struct backend *bdev = vhd_vdev_get_priv(req.vdev);
            struct iocb *ios = prepare_io_operation(&req);
            int ret;

            /* ret can be: 1 (success), 0 (some error), < 0 (errno) */
            do {
                ret = io_submit(bdev->io_ctx, 1, &ios);
            } while (ret == -EAGAIN);

            if (ret != 1) {
                PERROR("io_submit", -ret);
                complete_request(ios->data, VHD_BDEV_IOERR);
            }
        }
    }

    return NULL;
}

static sig_atomic_t stop_completion_thread;

static void thread_exit()
{
    stop_completion_thread = 1;
}

/*
 * IO requests completion handler thread, that serve one backend.
 */
static void *io_completion(void *thread_data)
{
    struct backend *bdev = thread_data;
    struct io_event events[MAX_AIO_EVENTS];

    /* setup a signal handler for thread stopping */
    struct sigaction sigusr1_action = {
        .sa_handler = thread_exit,
    };
    sigaction(SIGUSR1, &sigusr1_action, NULL);

    while (true) {
        int ret = io_getevents(bdev->io_ctx, 1, MAX_AIO_EVENTS, events, NULL);

        if (ret < 0 && ret != -EINTR) {
            DIE("io_getevents: %s", strerror(-ret));
        }

        if (stop_completion_thread) {
            break;
        }

        for (int i = 0; i < ret; i++) {
            struct request *req = events[i].data;
            vhd_log_stderr(LOG_DEBUG,
                           "IO result event for request with addr: %p", req);

            if ((events[i].res2 != 0) ||
                (events[i].res != req->bio->total_sectors * VHD_SECTOR_SIZE)) {
                complete_request(req, VHD_BDEV_IOERR);
                PERROR("IO request", -events[i].res);
            } else {
                if (bdev->delay) {
                    usleep(bdev->delay);
                }
                complete_request(req, VHD_BDEV_SUCCESS);
                vhd_log_stderr(LOG_DEBUG, "IO request completed successfully");
            }
            free(req);
        }
    }

    return NULL;
}

/*
 * Prepare backend before server starts.
 */
static int init_backend(struct backend *bdev, const struct backend_config *conf)
{
    off64_t file_len;
    int ret = 0;
    int flags = (conf->readonly ? O_RDONLY : O_RDWR) | O_DIRECT;

    bdev->fd = open(conf->blk_file, flags);
    if (bdev->fd < 0) {
        ret = errno;
        PERROR("open", ret);
        return -ret;
    }

    file_len = lseek(bdev->fd, 0, SEEK_END);
    if (file_len % VHD_SECTOR_SIZE != 0) {
        vhd_log_stderr(LOG_WARNING,
                       "File size is not a multiple of the block size");
        vhd_log_stderr(LOG_WARNING,
                       "Last %d bytes will not be accessible",
                       file_len % VHD_SECTOR_SIZE);
    }

    bdev->info.socket_path = conf->socket_path;
    bdev->info.serial = conf->serial;
    bdev->info.block_size = VHD_SECTOR_SIZE;
    bdev->info.num_queues = 256; /* Max count of virtio queues */
    bdev->info.total_blocks = file_len / VHD_SECTOR_SIZE;
    bdev->info.map_cb = NULL;
    bdev->info.unmap_cb = NULL;
    bdev->info.readonly = conf->readonly;
    bdev->delay = conf->delay;

    ret = -io_setup(MAX_AIO_QUEUE_LEN, &bdev->io_ctx);
    if (ret != 0) {
        PERROR("io_setup", ret);
        goto fail;
    }

    return 0;

fail:
    close(bdev->fd);
    return -ret;
}

/*
 * Register backend for existing queue and add backend to it.
 */
static int append_backend(struct queue *qdev, struct backend *bdev)
{
    qdev->bdev = bdev;

    bdev->handler = vhd_register_blockdev(&bdev->info, qdev->rq, bdev);
    if (!bdev->handler) {
        vhd_log_stderr(LOG_ERROR,
                       "vhd_register_blockdev: Can't register device");
        return -EINVAL;
    }

    return 0;
}

/*
 * Show usage message.
 */
static void usage(const char *cmd)
{
    printf("Usage: %s -s SOCKPATH -b FILEPATH -i SERIAL\n", cmd);
    printf("Start vhost daemon.\n");
    printf("\n");
    printf("Mandatory arguments to long options "
           "are mandatory for short options too.\n");
    printf("  -s, --socket-path=PATH  vhost-user Unix domain socket path\n");
    printf("  -i, --serial=STRING     disk serial\n");
    printf("  -b, --blk-file=PATH     block device or file path\n");
    printf("  -r, --readonly          readonly block device\n");
    printf("  -d msec, --delay=msec, delay of each completion request in microseconds\n");
}

/*
 * Parse command line options.
 */
static void parse_opts(int argc, char **argv, struct backend_config *conf)
{
    int opt;
    do {
        static struct option long_options[] = {
            {"socket-path",    1, NULL, 's'},
            {"serial",         1, NULL, 'i'},
            {"blk-file",       1, NULL, 'b'},
            {"delay",          1, NULL, 'd'},
            {"readonly",       0, NULL, 'r'},
            {0, 0, 0, 0}
        };

        opt = getopt_long(argc, argv, "s:i:b:d:r:", long_options, NULL);

        switch (opt) {
        case -1:
            break;
        case 's':
            conf->socket_path = optarg;
            break;
        case 'i':
            conf->serial = optarg;
            break;
        case 'b':
            conf->blk_file = optarg;
            break;
        case 'd':
            conf->delay = atoll(optarg);
            break;
        case 'r':
            conf->readonly = true;
            break;
        default:
            usage(argv[0]);
            exit(2);
        }
    } while (opt != -1);
}

static void notify_event(void *opaque)
{
    int *fd = (int *) opaque;

    while (eventfd_write(*fd, 1) && errno == EINTR) {
        ;
    }
}

static void wait_event(int fd)
{
    eventfd_t unused;

    while (eventfd_read(fd, &unused) && errno == EINTR) {
        ;
    }
}

/*
 * Main execution thread. Used for initializing and common management.
 */
int main(int argc, char **argv)
{
    struct backend bdev = {};
    struct queue qdev = {};
    pthread_t io_completion_thread, rq_thread;
    struct backend_config conf = {};
    sigset_t sigset;
    int sig, unreg_done_fd;

    parse_opts(argc, argv, &conf);

    if (!conf.socket_path || !conf.blk_file || !conf.serial) {
        usage(argv[0]);
        DIE("Invalid command line options");
    }

    if (init_backend(&bdev, &conf) < 0) {
        DIE("init_backend failed");
    }

    qdev.rq = vhd_create_request_queue();
    if (!qdev.rq) {
        DIE("vhd_create_request_queue failed");
    }

    if (vhd_start_vhost_server(vhd_log_stderr) < 0) {
        DIE("vhd_start_vhost_server failed");
    }

    if (append_backend(&qdev, &bdev) < 0) {
        DIE("append_backend failed");
    }

    /* tune the signal to block on waiting for "stop server" command */
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    pthread_sigmask(SIG_BLOCK, &sigset, NULL);

    vhd_log_stderr(LOG_INFO, "Test server started");

    /* start the worker thread(s) */
    pthread_create(&io_completion_thread, NULL, io_completion, &bdev);

    /* start libvhost request queue runner thread */
    pthread_create(&rq_thread, NULL, io_handle, &qdev);

    /* wait for signal to stop the server (Ctrl+C) */
    sigwait(&sigset, &sig);

    vhd_log_stderr(LOG_INFO, "Stopping the server");

    /* stopping sequence - should be in this order */

    /* 1. Unregister the blockdevs */
    /* 1.1 Prepare unregestring done event */
    unreg_done_fd = eventfd(0, 0);
    if (unreg_done_fd == -1) {
        DIE("eventfd creation failed");
    }
    /* 1.2. Call for unregister blockdev */
    vhd_unregister_blockdev(bdev.handler, notify_event, &unreg_done_fd);

    /* 1.3. Wait until the unregestering finishes */
    wait_event(unreg_done_fd);

    /* 2. Stop request queues. For each do: */
    /* 2.1 Stop a request queue */
    vhd_stop_queue(qdev.rq);

    /* 2.2 Wait for queue's thread to join */
    pthread_join(rq_thread, NULL);

    /* 3. Stop the worker thread(s) */
    pthread_kill(io_completion_thread, SIGUSR1);
    pthread_join(io_completion_thread, NULL);

    /* 4. Release request queues and stop the vhost server in any order */
    vhd_release_request_queue(qdev.rq);
    vhd_stop_vhost_server();

    /* 5. Release related resources */
    io_destroy(bdev.io_ctx);
    close(bdev.fd);

    vhd_log_stderr(LOG_INFO, "Server has been stopped.");

    return 0;
}
