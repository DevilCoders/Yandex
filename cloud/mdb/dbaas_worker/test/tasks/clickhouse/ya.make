PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PY_NAMESPACE(test.tasks.clickhouse)

PY_SRCS(utils.py)

PEERDIR(
    cloud/mdb/dbaas_worker/test/tasks
)

END()

RECURSE(
    cluster
    cluster/database
    cluster/dictionary
    cluster/format_schema
    cluster/model
    cluster/user
    host
    resetup
    shard
)
