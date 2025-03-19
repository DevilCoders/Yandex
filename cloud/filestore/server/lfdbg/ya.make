PROGRAM(filestore-server)

OWNER(g:cloud-nbs)

ALLOCATOR(LF_DBG)

CFLAGS(-DPROFILE_MEMORY_ALLOCATIONS)

SRCDIR(${ARCADIA_ROOT}/cloud/filestore/server)
INCLUDE(${ARCADIA_ROOT}/cloud/filestore/server/sources.inc)

END()
