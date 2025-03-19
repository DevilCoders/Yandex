UNITTEST()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/testing/unittest
    kernel/common_server/library/startrek
)

TAG(
    ya:external
    ya:force_sandbox
    ya:fat
)

SIZE(LARGE)

REQUIREMENTS(
    sb_vault:TESTING_STARTREK_TOKEN_PATH=file:EXTMAPS:carsharing-testing-startrek-token
    network:full
)

SRCS(
    startrek_ut.cpp
)

END()
