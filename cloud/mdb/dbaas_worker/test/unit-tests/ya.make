PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

NO_DOCTESTS()

SIZE(small)

FORK_SUBTESTS()

PEERDIR(
    cloud/mdb/dbaas_worker/internal
    cloud/mdb/internal/python/pytest
    contrib/python/pytest-mock
    contrib/python/PyHamcrest
    contrib/python/httmock
    contrib/python/mock
)

DATA(arcadia/cloud/mdb/dbaas_worker)

TEST_SRCS(
    common/cluster/test_delete_metadata.py
    common/test_create.py
    common/test_deploy.py
    kafka/cluster/test_modify.py
    clickhouse/test_zero_copy.py
    providers/dns/yc/test_upsert.py
    providers/dns/yc/test_provider.py
    providers/dns/test_dns.py
    providers/dns/test_provider.py
)

END()
