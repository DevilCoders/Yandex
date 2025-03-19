PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    library/python/testing/yatest_common
    cloud/mdb/salt/salt/components/pushclient2/parsers
)

TEST_SRCS(test_greenplum_csv_parser.py)

DATA(arcadia/cloud/mdb/salt-tests/components/pushclient/test_data/test_greenplum_csv_parser_data.txt)

TIMEOUT(60)

SIZE(SMALL)

END()
