PY2TEST()

OWNER(g:antiadblock)

TEST_SRCS(
    __init__.py
    test_config.py
    test_sonar.py
)

PEERDIR(
    contrib/python/PyHamcrest
    contrib/python/pytest-localserver
    antiadblock/adblock_rule_sonar/sonar/lib
)

END()
