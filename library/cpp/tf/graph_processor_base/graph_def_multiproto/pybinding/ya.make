OWNER(
    alexmir0x1
    g:factordev
)

PY23_LIBRARY()

PY_SRCS(
    multiproto.pyx
)

PEERDIR(
    library/cpp/tf/graph_processor_base/graph_def_multiproto
)

END()
