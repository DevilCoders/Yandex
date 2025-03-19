OWNER(g:cloud-dwh)

PY3_PROGRAM()

PEERDIR(
    cloud/dwh/utils

    cloud/dwh/nirvana/operations/solomon_to_yt/lib
)

PY_SRCS(
    MAIN main.py
)

END()

RECURSE(
    lib
)
