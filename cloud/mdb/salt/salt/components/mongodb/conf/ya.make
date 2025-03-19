PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PY_SRCS(
    mdb_disk_watcher.py
    mdb_mongodb_deadlock_detector.py
)

PEERDIR(
    contrib/python/pymongo
)

NEED_CHECK()

END()

RECURSE_ROOT_RELATIVE(
    cloud/mdb/salt-tests/components/mongodb
)
