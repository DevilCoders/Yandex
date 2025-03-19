PROGRAM(filestore-vhost)

OWNER(g:cloud-nbs)

ALLOCATOR(LF_DBG)

CFLAGS(-DPROFILE_MEMORY_ALLOCATIONS)

SRCDIR(${ARCADIA_ROOT}/cloud/filestore/vhost)
INCLUDE(${ARCADIA_ROOT}/cloud/filestore/vhost/sources.inc)

END()
