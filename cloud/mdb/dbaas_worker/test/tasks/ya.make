PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PY_NAMESPACE(test.tasks)

PEERDIR(
    cloud/mdb/internal/python/compute/disks
    contrib/python/PyHamcrest
    cloud/mdb/dbaas_worker/internal
    cloud/mdb/dbaas_worker/test/mocks
)

PY_SRCS(utils.py)

END()
