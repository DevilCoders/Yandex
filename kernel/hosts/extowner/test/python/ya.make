PY2TEST()

OWNER(
    karina-usm
    g:antiinfra
)

SRCDIR(kernel/hosts/extowner/test)

TEST_SRCS(
    extowner_perf_test.py
)

DEPENDS(
    kernel/hosts/extowner/test
)

SIZE(LARGE)
TAG(
    ya:force_sandbox
    ya:perftest
    ya:fat
)

DATA(
    sbr://985478017=urls
    arcadia/yweb/urlrules/areas.lst
)

END()
