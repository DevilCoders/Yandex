PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    cloud/mdb/internal/python/pytest
    cloud/mdb/salt/salt/components/mongodb/conf
    cloud/mdb/salt-tests/common
    contrib/python/mock
)

TEST_SRCS(
    mdb_disk_watcher.py
    mdb_disk_watcher_mocks.py
    mdb_mongodb_deadlock_detector.py
    mdb_mongodb_deadlock_detector_mocks.py
)

TIMEOUT(60)

SIZE(SMALL)

END()
