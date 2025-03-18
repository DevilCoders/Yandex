GO_TEST_FOR(library/go/yolint/internal/passes/protonaming)

OWNER(
    g:go-library
    gzuykov
)

DATA(arcadia/library/go/yolint/internal/passes/protonaming/testdata)

TEST_CWD(library/go/yolint/internal/passes/protonaming)

INCLUDE(${ARCADIA_ROOT}/library/go/test/go_toolchain/recipe.inc)

END()
