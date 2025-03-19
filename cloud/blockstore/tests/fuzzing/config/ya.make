OWNER(g:cloud-nbs)

PY3_PROGRAM(config-recipe)

PY_SRCS(__main__.py)

PEERDIR(
    cloud/blockstore/config
    cloud/blockstore/tests/python/lib
    kikimr/ci/libraries
    library/python/testing/recipe
    library/python/testing/yatest_common
    ydb/core/protos
)

DEPENDS(kikimr/driver)

END()
