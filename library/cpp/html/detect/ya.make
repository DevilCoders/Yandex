OWNER(g:cpp-contrib)

LIBRARY()

PEERDIR(
    library/cpp/html/face
)

SRCS(
    detect.cpp
    mset.cpp
    attrs.gperf
    ${CURDIR}/machine.rl6
)

SET(
    RAGEL6_FLAGS
    -T1
    -L
)

END()
