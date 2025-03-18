PY2TEST()

OWNER(
    g:marketsre
)

PEERDIR(
    tools/python-netconfig-static/src
)

TEST_SRCS(
    test_mtu.py
    test_tunnel.py
    utils.py
)

END()
