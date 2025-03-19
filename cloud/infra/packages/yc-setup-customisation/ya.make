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
    tests/test_configure_network.py
    tests/test_nvme_part_label.py
)

END()
