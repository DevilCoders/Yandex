#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <stdexcept>
#include <thread>
#include <atomic>
#include <list>
#include <set>
#include <deque>
#include <fstream>
#include <memory>

#include "vhost/server.h"
#include "vhost/blockdev.h"
#include "platform.h"

#include "test_utils.h"

#define SERVER_LOG(level, fmt, ...)                 \
do {                                                \
        fprintf(stderr, level ": %s:%d: " fmt "\n", \
                __func__, __LINE__, ##__VA_ARGS__); \
} while (0)

#ifdef VHD_DEBUG
#   define SERVER_LOG_DEBUG(fmt, ...) SERVER_LOG("DEBUG", fmt, ##__VA_ARGS__)
#else
#   define SERVER_LOG_DEBUG(fmt, ...)
#endif

#define SERVER_LOG_INFO(fmt, ...)     SERVER_LOG("INFO", fmt, ##__VA_ARGS__)
#define SERVER_LOG_WARN(fmt, ...)     SERVER_LOG("WARN", fmt, ##__VA_ARGS__)
#define SERVER_LOG_ERROR(fmt, ...)    SERVER_LOG("ERROR", fmt, ##__VA_ARGS__)

#define SERVER_LOG_TRACE()            SERVER_LOG_DEBUG("")

#define DIE(fmt, ...)                     \
do {                                      \
    SERVER_LOG_ERROR(fmt, ##__VA_ARGS__); \
    exit(EXIT_FAILURE);                   \
} while (0)

/*
 * The /tmp/vhost.cfg file is just a file with 4 numbers:
 *   0 0 0 0
 * 1. Delay in seconds before starting to process requests.
 * 2. Delay in seconds before starting to complete requests by updating
 * used vring.
 * 3. On which completion number we should stop running vhost-server daemon.
 * 4. Use ascending or descending order to complete the requests in the
 * queue. 0 - ascending (default), 1 - descending.
 *
 * This file is used to change the vhost-server request completion
 * handling for the inflight testing purposes.
 */
#define VHOST_CFG_PATH "/tmp/vhost.cfg"

static bool is_power_of_two64(uint64_t x)
{
    return (x & (x - 1)) == 0;
}

/*////////////////////////////////////////////////////////////////////////////*/

/**
 * Simple file-based bdev backend
 */
class file_bdev final
{
public:

    file_bdev() = default;
    ~file_bdev()
    {
        this->close();
    }

    file_bdev(const file_bdev &) = delete;
    file_bdev &operator=(const file_bdev &) = delete;

    int open(const char *path, const char *sock, uint64_t blocksize);
    void start(vhd_request_queue *rq);
    void close();

    void handle_io(vhd_bdev_io *bio);
    void complete_io();
    void reread_cfg();

private:

    vhd_bdev_info m_bdev_info;
    vhd_vdev *m_vdev_handle = nullptr;
    int m_fd = -1;
    std::deque<std::pair<vhd_bdev_io *, enum vhd_bdev_io_result>> m_inflight;

    // Delay in seconds before starting to process requests.
    int m_predelay = 0;
    // Delay in seconds before starting to complete requests.
    int m_inflightdelay = 0;
    // On which completion number we should stop running vhost-server daemon.
    int m_simabort = 0;
    // Use ascending or descending order to complete the requests in the
    // queue. 0 - ascending (default), 1 - descending.
    int m_simorder = 0;
};

int file_bdev::open(const char *path, const char *sock, uint64_t blocksize)
{
    int ret = 0;

    if (!is_power_of_two64(blocksize)) {
        SERVER_LOG_ERROR("blocksize must be a power of 2");
        return -EINVAL;
    }

    int fd = ::open(path, O_SYNC | O_RDWR);
    if (fd < 0) {
        SERVER_LOG_ERROR("could not open \"%s\": %d", path, errno);
        return -errno;
    }

    struct stat stbuf;
    ret = fstat(fd, &stbuf);
    if (ret != 0) {
        SERVER_LOG_ERROR("could not stat %d: %d", fd, errno);
        return -errno;
    }

    assert(is_power_of_two64(stbuf.st_blksize));

    if (blocksize & (stbuf.st_blksize - 1)) {
        SERVER_LOG_ERROR("blocksize %llu should be a multiple of unerlying FS block size %llu",
                (unsigned long long)blocksize,
                (unsigned long long)stbuf.st_blksize);
        return -EINVAL;
    }

    uint64_t total_blocks = stbuf.st_size / blocksize;

    m_fd = fd;
    m_bdev_info.socket_path = sock;
    m_bdev_info.serial = "libvhost_disk_serial";
    m_bdev_info.block_size = blocksize;
    m_bdev_info.total_blocks = total_blocks;
    m_bdev_info.num_queues = 1;
    m_bdev_info.map_cb = NULL;
    m_bdev_info.unmap_cb = NULL;

    return 0;
}

void file_bdev::start(vhd_request_queue *rq)
{
    /* Now we're public, requests can start going in */
    vhd_vdev *vdev = vhd_register_blockdev(&m_bdev_info, rq, this);
    VHD_VERIFY(vdev != nullptr);

    m_vdev_handle = vdev;
}

void file_bdev::close()
{
    if (m_vdev_handle) {
        vhd_unregister_blockdev(m_vdev_handle, NULL, NULL);
        m_vdev_handle = nullptr;
    }

    if (m_fd != -1) {
        ::close(m_fd);
        m_fd = -1;
    }
}

void file_bdev::handle_io(vhd_bdev_io *bio)
{
    uint64_t offset = bio->first_sector * VHD_SECTOR_SIZE;
    uint64_t total_size = bio->total_sectors * VHD_SECTOR_SIZE;

    VHD_VERIFY(VHD_IS_ALIGNED(offset, m_bdev_info.block_size));
    VHD_VERIFY(VHD_IS_ALIGNED(total_size, m_bdev_info.block_size));

    uint64_t block = offset / m_bdev_info.block_size;
    uint64_t total_blocks = total_size / m_bdev_info.block_size;

    vhd_buffer *pbuf = bio->sglist.buffers;
    vhd_bdev_io_result iores = VHD_BDEV_SUCCESS;

    SERVER_LOG_DEBUG("request %p: block %llu, total blocks %llu, type %s",
            bio,
            (unsigned long long)block,
            (unsigned long long)total_blocks,
            bio->type == VHD_BDEV_READ ? "read" : "write");

    if (m_predelay) {
        sleep(m_predelay);
    }
    for (uint32_t i = 0; i < bio->sglist.nbuffers; ++i) {
        VHD_VERIFY(total_blocks > 0);
        VHD_VERIFY((pbuf->len & (m_bdev_info.block_size - 1)) == 0);

        size_t nbytes = std::min(total_blocks * m_bdev_info.block_size,
                                 pbuf->len);
        ssize_t res = 0;

        if (bio->type == VHD_BDEV_READ) {
            res = pread(m_fd, pbuf->base, nbytes,
                        block * m_bdev_info.block_size);
        } else {
            res = pwrite(m_fd, pbuf->base, nbytes,
                         block * m_bdev_info.block_size);
        }

        if (res < 0) {
            SERVER_LOG_ERROR("pread/pwrite failed with %zd", res);
            iores = VHD_BDEV_IOERR;
            break;
        }

        block += nbytes / m_bdev_info.block_size;
        total_blocks -= nbytes / m_bdev_info.block_size;

        ++pbuf;
    }

    // Push request to the inflight queue, so we can postpone sending of completion
    // and test inflight-reconnect functionality.
    m_inflight.push_back({bio, iores});
}

void file_bdev::complete_io()
{
    bool ascending = true;
    int cnt = 0;

    SERVER_LOG_DEBUG("number of inflight requests: %lu", m_inflight.size());
    if (m_inflightdelay) {
        sleep(m_inflightdelay);
    }

    if (m_simorder) {
        ascending = false;
    }

    while (m_inflight.size()) {
        std::pair<vhd_bdev_io *, enum vhd_bdev_io_result> item;
        if (ascending) {
            item = m_inflight.front();
            m_inflight.pop_front();
        } else {
            item = m_inflight.back();
            m_inflight.pop_back();
        }
        SERVER_LOG_DEBUG("request %p: completing with %d", item.first,
                         item.second);
        vhd_complete_bio(item.first, item.second);
        cnt++;
        if (cnt == m_simabort) {
            SERVER_LOG_DEBUG("simulate vhost-server crash");
            exit(-1);
        }
    }
}

void file_bdev::reread_cfg()
{
    SERVER_LOG_DEBUG("Reread config");

    std::ifstream cfg;

    cfg.open(VHOST_CFG_PATH);
    if (!cfg) {
        SERVER_LOG_DEBUG("Can't open file: %s", VHOST_CFG_PATH);
        return;
    }

    cfg >> m_predelay >> m_inflightdelay >> m_simabort >> m_simorder;
    cfg.close();

    SERVER_LOG_DEBUG("New cfg values:");
    SERVER_LOG_DEBUG("\tm_predelay = %d", m_predelay);
    SERVER_LOG_DEBUG("\tm_inflightdelay = %d", m_inflightdelay);
    SERVER_LOG_DEBUG("\tm_simabort = %d", m_simabort);
    SERVER_LOG_DEBUG("\tm_simorder = %d", m_simorder);
}

/*////////////////////////////////////////////////////////////////////////////*/

/**
 * Vhost server with 1 request queue
 */
class vhost_server final
{
public:

    static std::shared_ptr<vhost_server> start()
    {
        return std::make_shared<vhost_server>();
    }

    vhost_server();
    ~vhost_server()
    {
        this->stop();
    }

    vhost_server(const vhost_server &) = delete;
    vhost_server &operator=(const vhost_server &) = delete;

    void register_bdev(file_bdev *bdev);
    void stop();

private:

    // Executes in worker thread
    void run();

private:

    vhd_request_queue *m_rq = nullptr;
    std::thread m_worker;
    std::atomic<bool> m_should_stop;
    std::list<file_bdev *> m_bdevs;
};

vhost_server::vhost_server() : m_should_stop(false)
{
    int res = 0;

    m_rq = vhd_create_request_queue();
    if (!m_rq) {
        throw std::runtime_error("could not create request queue");
    }

    res = vhd_start_vhost_server(vhd_log_stderr);
    if (res != 0) {
        vhd_release_request_queue(m_rq);
        throw std::runtime_error("could not start vhost server");
    }

    m_worker = std::thread([&]() {
        this->run();
    });
}

void vhost_server::register_bdev(file_bdev *bdev)
{
    bdev->start(m_rq);
    m_bdevs.push_front(bdev);
}

void vhost_server::stop()
{
    /* Close devices before the queue stopping */
    while (!m_bdevs.empty()) {
        file_bdev *bdev = m_bdevs.front();
        bdev->close();
        m_bdevs.pop_front();
    }

    if (!m_worker.joinable()) {
        return;
    }

    /* Interrupt request queue and wait for worker thread to exit */
    m_should_stop.store(true);
    vhd_stop_queue(m_rq);
    m_worker.join();

    /* Stop vhost server to avoid getting new vhost events */
    vhd_stop_vhost_server();

    /* Safely release request queue */
    vhd_release_request_queue(m_rq);
}

void vhost_server::run()
{
    while (!m_should_stop.load()) {
        int res = vhd_run_queue(m_rq);
        if (res != -EAGAIN) {
            if (res < 0) {
                throw std::runtime_error("request queue failure");
            } else {
                return;
            }
        }

        vhd_request req;
        std::set<file_bdev *> bdev_set;
        while (vhd_dequeue_request(m_rq, &req)) {
            file_bdev *bdev = reinterpret_cast<file_bdev *>(
                vhd_vdev_get_priv(req.vdev));
            VHD_VERIFY(bdev);

            bdev->handle_io(req.bio);
            bdev_set.insert(bdev);
        }
        for (const auto item : bdev_set) {
            item->complete_io();
        }
    }
}

/*////////////////////////////////////////////////////////////////////////////*/

static volatile bool g_term = false;
static volatile bool g_rereadcfg = false;

/* Show usage message. */
static void Usage(const char *cmd)
{
    printf("Usage: %s -s SOCKPATH -f FILEPATH -b NBLOCKS\n", cmd);
    printf("Start vhost daemon.\n");
    printf("\n");
    printf("The list of options are as follows:\n");
    printf("  -s SOCKPATH                set path to the named socket\n");
    printf("  -f FILEPATH                set path to the block device file\n");
    printf("  -b BLOCKSIZE               logical block size in bytes\n");
}

static void handle_term(int sig)
{
    SERVER_LOG_WARN("Terminating on signal %d", sig);
    g_term = true;
}

static void handle_sigusr1(__attribute__((unused)) int sig)
{
    SERVER_LOG_DEBUG("SIGUSR1 to reread config");
    g_rereadcfg = true;
}

int main(int argc, char *argv[])
{
    int ret;
    const char *sockpath = nullptr;
    const char *filepath = nullptr;
    uint64_t blocksize = 0;

    char opt;
    while ((opt = getopt(argc, argv, "s:f:b:")) != -1) {
        switch (opt) {
        case 's':
            sockpath = optarg;
            break;
        case 'f':
            filepath = optarg;
            break;
        case 'b':
            blocksize = strtoull(optarg, nullptr, 0);
            break;
        default:
            Usage(argv[0]);
            DIE("Unknown command line option %c", opt);
        }
    }

    if (!sockpath || !filepath || !blocksize) {
        Usage(argv[0]);
        DIE("Invalid command line options");
    }

    file_bdev bdev;
    ret = bdev.open(filepath, sockpath, blocksize);
    if (ret != 0) {
        DIE("could not open bdev");
    }

    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = handle_term;
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGINT, &action, NULL);
    action.sa_handler = handle_sigusr1;
    sigaction(SIGUSR1, &action, NULL);

    std::shared_ptr<vhost_server> server = vhost_server::start();
    server->register_bdev(&bdev);

    while (!g_term) {
        sleep(1);
        if (g_rereadcfg) {
            bdev.reread_cfg();
            g_rereadcfg = false;
        }
    }

    SERVER_LOG_INFO("Exiting");
    return ret;
}
