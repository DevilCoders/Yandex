GO_TEST_FOR(library/go/yolint/internal/passes/importcheck)

OWNER(
    gzuykov
    g:go-library
)

INCLUDE(${ARCADIA_ROOT}/library/go/test/go_toolchain/recipe.inc)

DATA(arcadia/library/go/yolint/internal/passes/importcheck/testdata)

TEST_CWD(library/go/yolint/internal/passes/importcheck)

END()
