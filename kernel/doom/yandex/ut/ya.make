UNITTEST_FOR(kernel/doom/yandex)

ALLOCATOR(LF)

OWNER(
    mvel
    g:base
)

PEERDIR(
    library/cpp/archive
    kernel/doom/hits
    kernel/doom/yandex
)

FROM_SANDBOX(135769559 OUT unsorted_index.inv)

FROM_SANDBOX(135769452 OUT unsorted_index.key)

ARCHIVE_ASM(
    NAME UnsortedIndex
    DONTCOMPRESS
    unsorted_index.inv
    unsorted_index.key
)

SRCS(
    unsorted_yandex_reader_ut.cpp
)

END()
