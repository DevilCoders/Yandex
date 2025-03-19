PY3_PROGRAM(yc-nbs-ci-checkpoint-validation-test)

OWNER(g:cloud-nbs)

PY_SRCS(
    __main__.py
)

RESOURCE(
    configs/nbs-client.txt nbs-client.txt
    configs/throttler-profile.txt throttler-profile.txt
)

PEERDIR(
    cloud/blockstore/public/sdk/python/client

    cloud/blockstore/pylibs/common
    cloud/blockstore/pylibs/sdk
    cloud/blockstore/pylibs/ycp

    cloud/blockstore/tools/ci/checkpoint_validation_test/lib
)

END()

RECURSE(
    lib
)

RECURSE_FOR_TESTS(
    tests
)

