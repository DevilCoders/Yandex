UNITTEST_FOR(cloud/blockstore/libs/common)

OWNER(g:cloud-nbs)

IF (WITH_VALGRIND)
    TIMEOUT(600)
    SIZE(MEDIUM)
ENDIF()

# this test sometimes times out under tsan
# in fact, there is no need to run it under tsan - the logic is single-threaded
IF (SANITIZER_TYPE != "thread")
    SRCS(
        compressed_bitmap_ut.cpp
    )
ENDIF()

SRCS(
    caching_allocator_ut.cpp
    block_buffer_ut.cpp
    block_checksum_ut.cpp
    block_data_ref_ut.cpp
    block_range_ut.cpp
    guarded_sglist_ut.cpp
    iovector_ut.cpp
    leaky_bucket_ut.cpp
    sglist_ut.cpp
)

END()
