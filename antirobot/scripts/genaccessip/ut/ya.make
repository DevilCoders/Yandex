OWNER(g:antirobot)

PY2TEST()

TEST_SRCS(
    test.py
)

PEERDIR(
    antirobot/scripts/genaccessip
)

REQUIREMENTS(network:full)

DATA(
    arcadia/antirobot/scripts/support/privileged_ips
    arcadia/antirobot/scripts/support/special_ips.txt
    arcadia/antirobot/scripts/support/trbosrvnets.txt
    arcadia/antirobot/scripts/support/whitelist_ips.txt
    arcadia/antirobot/scripts/support/yandex_ips.txt
)

SIZE(MEDIUM)

END()
