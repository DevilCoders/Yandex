GO_TEST_FOR(library/go/yolint/internal/passes/errcheck)

OWNER(
    g:go-library
    gzuykov
)

DATA(arcadia/library/go/yolint/internal/passes/errcheck/testdata)

TEST_CWD(library/go/yolint/internal/passes/errcheck)

INCLUDE(${ARCADIA_ROOT}/library/go/test/go_toolchain/recipe.inc)

END()
