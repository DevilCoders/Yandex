PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PY_NAMESPACE(test.tasks.kafka)

PY_SRCS(utils.py)

PEERDIR(
    cloud/mdb/dbaas_worker/test/tasks
)

END()

RECURSE(
    cluster
    cluster/connector
    cluster/topic
    cluster/user
    host
)
