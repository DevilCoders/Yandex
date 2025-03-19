LIBRARY()

OWNER(
    platovd
    g:blender
)

PEERDIR(
    kernel/extended_mx_calcer/interface
    kernel/extended_mx_calcer/proto
)

SRCS(
    multipredict.cpp
    scored_categs.cpp
)

END()
