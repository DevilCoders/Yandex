OWNER(g:mdb)

PY3TEST()

STYLE_PYTHON()

SIZE(SMALL)

PEERDIR(
    cloud/mdb/idm_service/idm_service
)

TEST_SRCS(
    test_dummy.py
    test_mysql_pillar.py
    test_mysql_pillar_resps.py
    test_pg_pillar.py
    test_pg_pillar_resps.py
)

END()
