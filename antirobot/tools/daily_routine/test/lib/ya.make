PY2TEST()

OWNER(
    g:antirobot
)

TEST_SRCS(
    test_stream_converters.py
)

PEERDIR(
    statbox/nile
    statbox/qb2
    antirobot/tools/daily_routine/lib
)

NO_CHECK_IMPORTS(qb2_extensions.*)

FORK_TESTS()
FORK_SUBTESTS()

END()
