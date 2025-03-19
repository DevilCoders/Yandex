LIBRARY()

OWNER(
    elric
    mvel
    g:base
)

SRCS(
    sequential_yandex_reader.h
    unsorted_yandex_reader.h
    yandex_io.h
    yandex_key_data.h
    yandex_reader.h
    yandex_writer.h
    yandex_key_searcher.h
    yandex_key_iterator.h
    yandex_hit_searcher.h
    yandex_hit_iterator.h
    dummy.cpp
)

PEERDIR(
    kernel/doom/hits
    kernel/doom/progress
    kernel/keyinv/hitlist
    kernel/keyinv/indexfile
)

END()
