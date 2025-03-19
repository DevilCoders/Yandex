OWNER(frystile)

PY2_PROGRAM(recipe)

PY_SRCS(__main__.py)

PEERDIR(
    library/python/testing/recipe
    library/python/testing/yatest_common
)

FROM_SANDBOX(FILE 1369913138 OUT_NOAUTO snapshot-qemu-nbd-docker-image.tar)
DATA(sbr://1369913138)

END()

