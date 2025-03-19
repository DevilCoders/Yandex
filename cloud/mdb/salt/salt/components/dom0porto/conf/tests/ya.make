OWNER(g:mdb)

PY2TEST()


PEERDIR(
    cloud/mdb/salt/salt/components/dom0porto/conf
    contrib/python/PyHamcrest
)

TEST_SRCS(
    test_heartbeat.py
)

DATA(arcadia/cloud/mdb/salt/salt/components/dom0porto/conf/tests)



END()

