OWNER(
    g:turbo
)

PY2TEST()

TIMEOUT(60)

TEST_SRCS(run_canonizer.py)

PEERDIR(kernel/turbo/pycanonizer)

SIZE(LARGE)

TAG(ya:fat)

END()
