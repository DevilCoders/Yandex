OWNER(g:cloud-nbs)

PY3_PROGRAM()

PY_SRCS(
    __main__.py
)

PEERDIR(
    cloud/disk_manager/test/recipe
    library/python/testing/recipe
)

END()
