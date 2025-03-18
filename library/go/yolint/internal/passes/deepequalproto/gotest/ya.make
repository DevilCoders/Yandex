GO_TEST_FOR(library/go/yolint/internal/passes/deepequalproto)

OWNER(
    g:go-library
    prime
)

DATA(arcadia/library/go/yolint/internal/passes/deepequalproto/testdata)

TEST_CWD(library/go/yolint/internal/passes/deepequalproto)

INCLUDE(${ARCADIA_ROOT}/library/go/test/go_toolchain/recipe.inc)

END()
