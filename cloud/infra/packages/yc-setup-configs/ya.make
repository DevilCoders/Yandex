OWNER(g:cloud-infra)

RECURSE(
    bin
)

PY3TEST()

PEERDIR(
    cloud/infra/packages/lib
    contrib/python/parameterized
)

TEST_SRCS(
    tests/test_manage_apt_repos.py
)

END()
