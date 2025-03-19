PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PY_NAMESPACE(test.tasks.opensearch)

PY_SRCS(utils.py)

PEERDIR(
    cloud/mdb/dbaas_worker/test/tasks
)

END()

RECURSE(
    cluster
    cluster/user
    cluster/host
)
