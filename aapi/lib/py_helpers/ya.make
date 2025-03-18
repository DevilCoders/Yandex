PY2_LIBRARY()

OWNER(g:aapi)

PEERDIR(
    aapi/lib/node
)

SRCS(helpers.cpp)

PY_SRCS(
    TOP_LEVEL
    vcs_helpers.pyx
)

END()
