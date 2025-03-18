GO_TEST_FOR(library/go/yolint/internal/passes/testifycheck)

OWNER(
    g:go-library
    prime
)

DATA(arcadia/library/go/yolint/internal/passes/testifycheck/testdata)

TEST_CWD(library/go/yolint/internal/passes/testifycheck)

INCLUDE(${ARCADIA_ROOT}/library/go/test/go_toolchain/recipe.inc)

END()
