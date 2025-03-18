UNITTEST_FOR(library/cpp/ssh)

OWNER(cerevra)

SRCS(
    sign_ut.cpp
)

PEERDIR(
    library/cpp/string_utils/base64
)

DATA(arcadia/library/cpp/ssh/ut/testdata)

IF (NOT OS_WINDOWS)
    SRCS(agent_ut.cpp)

    DEPENDS(library/cpp/ssh/ut/ssh_agent_recipe)
    USE_RECIPE(library/cpp/ssh/ut/ssh_agent_recipe/ssh_agent_recipe --keys library/cpp/ssh/ut/testdata/rsa_key --keys library/cpp/ssh/ut/testdata/ecdsa_key)
ENDIF()

END()

RECURSE(ssh_agent_recipe)
