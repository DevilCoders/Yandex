PY2_PROGRAM(qemu-recipe)

OWNER(g:cloud-nbs)

PY_SRCS(__main__.py)

PEERDIR(
    cloud/storage/core/tools/testing/qemu/lib

    library/python/testing/recipe
    library/python/testing/yatest_common
)

END()
