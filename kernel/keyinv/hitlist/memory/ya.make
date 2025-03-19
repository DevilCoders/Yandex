LIBRARY()

OWNER(
    kostik
    ivanmorozov
)

SRCS(
    rt_hits_block.cpp
    rt_hits_coders.cpp
    rt_hits_coders_gen.cpp
)

PEERDIR(
    kernel/keyinv/invkeypos
)

END()
