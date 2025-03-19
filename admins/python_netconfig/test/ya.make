PY2TEST()

PEERDIR(
    admins/python_netconfig/src
)

TEST_SRCS(
    test_mtu.py
    test_tunnel.py
    utils.py
)

END()
