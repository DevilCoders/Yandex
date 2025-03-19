UNITTEST_FOR(kernel/common_server/library/staff)

OWNER(g:cs_dev)

TAG(
    ya:external
    ya:force_sandbox
    ya:fat
)

SIZE(LARGE)

REQUIREMENTS(
    sb_vault:TESTING_STAFF_TOKEN_PATH=file:EXTMAPS:carsharing-testing-startrek-token
    network:full
)

SRCS(
    staff_ut.cpp
)

END()
