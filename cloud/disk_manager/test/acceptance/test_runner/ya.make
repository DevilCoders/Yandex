OWNER(g:cloud-nbs)

PY3_PROGRAM(yc-disk-manager-ci-acceptance-test-suite)

PY_SRCS(
    __main__.py
    acceptance_test_runner.py
    base_acceptance_test_runner.py
    eternal_acceptance_test_runner.py
)

PEERDIR(
    cloud/blockstore/pylibs/clients/solomon

    cloud/disk_manager/test/acceptance/test_runner/lib

    ydb/tests/library
)

END()

RECURSE(
    lib
)
