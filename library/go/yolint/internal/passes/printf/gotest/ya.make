GO_TEST_FOR(library/go/yolint/internal/passes/printf)

OWNER(
    buglloc
    g:go-library
)

DATA(arcadia/library/go/yolint/internal/passes/printf/testdata)

TEST_CWD(library/go/yolint/internal/passes/printf)

INCLUDE(${ARCADIA_ROOT}/library/go/test/go_toolchain/recipe.inc)

END()
