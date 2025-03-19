PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

FORK_SUBTESTS()

SIZE(MEDIUM)

PEERDIR(
    cloud/mdb/internal/python/grpcutil
    cloud/mdb/internal/python/compute
    cloud/mdb/internal/python/logs
    contrib/python/pytest-mock
)

DATA(arcadia/cloud/mdb/internal)

TEST_SRCS(
    grpcutil/service_test.py
    compute/paginate_test.py
    logs/context_test.py
    logs/format/json_test.py
)

REQUIREMENTS(cpu:4)

END()
