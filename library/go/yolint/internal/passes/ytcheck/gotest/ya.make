GO_TEST_FOR(library/go/yolint/internal/passes/ytcheck)

OWNER(
    g:go-library
    gzuykov
)

DATA(arcadia/library/go/yolint/internal/passes/ytcheck/testdata)

TEST_CWD(library/go/yolint/internal/passes/ytcheck)

INCLUDE(${ARCADIA_ROOT}/library/go/test/go_toolchain/recipe.inc)

END()
