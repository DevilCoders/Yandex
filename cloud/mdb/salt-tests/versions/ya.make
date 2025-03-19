PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    contrib/python/PyYAML
    library/python/testing/yatest_common
)

TEST_SRCS(
    test_pinned_versions.py
    test_versions_format.py
)

DATA(arcadia/cloud/mdb/salt/pillar/versions.sls)

DATA(arcadia/cloud/mdb/salt/salt/components)

TIMEOUT(60)

SIZE(SMALL)

REQUIREMENTS(ram:16)

END()
