GO_TEST_FOR(library/go/yolint/internal/passes/naming)

OWNER(
    gzuykov
    g:go-library
)

DATA(arcadia/library/go/yolint/internal/passes/naming/testdata)

TEST_CWD(library/go/yolint/internal/passes/naming)

INCLUDE(${ARCADIA_ROOT}/library/go/test/go_toolchain/recipe.inc)

END()
