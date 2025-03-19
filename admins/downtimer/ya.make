OWNER(
    g:music
)

PY2_PROGRAM(downtimer)

PY_MAIN(downtimer)

PY_SRCS(
    TOP_LEVEL
    downtimer.py
)

PEERDIR(
    contrib/python/juggler_sdk
    sandbox/projects/common/nanny
    yp/python/yp/client
)

END()
