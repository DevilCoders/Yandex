PY23_TEST()
OWNER(g:mdb)

PEERDIR(
    cloud/mdb/internal/python/pytest
    cloud/mdb/salt/salt/_modules
    cloud/mdb/salt-tests/common
    contrib/python/mock
)

TEST_SRCS(
    test_get_external_project_ids_with_action.py
    test_get_external_project_ids.py
    test_get_project_id.py
    test_ip6_interfaces.py
    test_ip6_porto.py
)

TIMEOUT(60)
SIZE(SMALL)
END()
