LIBRARY()

OWNER(
    g:base
)

SRCS(
    key_inv_printer.h
)

PEERDIR(
    kernel/doom/algorithm
    kernel/doom/info
    kernel/doom/key
    kernel/doom/offroad
    library/cpp/hnsw/index
    tools/idx_print/utils
    ysite/yandex/erf
)

END()
