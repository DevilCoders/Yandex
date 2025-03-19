UNITTEST()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/geometry
)

SRCDIR(kernel/common_server/library/geometry)

SRCS(
    algorithms_ut.cpp
    geometry_ut.cpp
    rect_ut.cpp
)

END()
