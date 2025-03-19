# Virtiofs-server

This is a simple virtiofs server that can pass a local directory through as a
filesystem to the guest.

## How to run

Increase open file limit

    ulimit -n 1048576

Run server

    virtiofs-server --socket-path=/tmp/fs.sock -o source=/tmp/fuse_src_dir

## Ingredients

**libvhost.a** - contains code for our vhost protocol implementation.

**libfuse.a** - patched libfuse that provides lowlevel FUSE interface.

**fuse_virtio.c** - glue that teaches libfuse.a to process requests obtained
from libvhost.a (imported from `arcadia/cloud/filestore/libs/fuse/vhost/`,
implementation is partial - doen't support `fuse_reply_data` because
`virtio_send_data_iov` is not implemented yet).

**passthrough_ll.c** - modified example from upstream `libfuse/examples` with
basic implementation of lowlevel FUSE callbacks (multythread support was dropped
to simplify server, `read` callback is rewritten to use `fuse_reply_buf` instead
of `fuse_reply_data`, and `O_DIRECT` flag is cleared for all files to avoid
temporary buffer alignment requirenents).

**helper.c** - helper functions, mostly for programm arguments handling (copied
from **qemu**).
