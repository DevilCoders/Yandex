PY3TEST()
OWNER(g:mdb)

DATA(
    arcadia/cloud/mdb/salt/salt/components/vector_agent_dataplane/conf
    arcadia/cloud/mdb/salt/salt/components/clickhouse/vector
    arcadia/cloud/mdb/salt/salt/components/kafka/conf/vector_kafka.toml
)


TEST_SRCS(
    test_vector.py
)

PEERDIR(
    contrib/python/Jinja2
)

DEPENDS(cloud/mdb/salt-tests/components/vector/vector)

TIMEOUT(60)
SIZE(SMALL)
END()

RECURSE(vector)
