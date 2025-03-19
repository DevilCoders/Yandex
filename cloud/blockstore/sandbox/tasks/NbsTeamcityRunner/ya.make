PY23_LIBRARY()

OWNER(g:cloud-nbs)

PY_SRCS(
    __init__.py
    task.py
)

PEERDIR(
    cloud/blockstore/sandbox/utils

    sandbox/sdk2
)

END()
