GO_TEST()

OWNER(
    g:go-library
    prime
)

DEPENDS(library/go/blockcodecs/integration/cpp)

IF (RACE)
    ENV(BLOCKCODECS_FLAT_TEST=1)
ENDIF()

SIZE(MEDIUM)

REQUIREMENTS(ram:32)

GO_TEST_SRCS(
    compat_test.go
    decoder_test.go
)

END()

RECURSE(cpp)
