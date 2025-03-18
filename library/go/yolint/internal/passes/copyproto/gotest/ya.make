GO_TEST_FOR(library/go/yolint/internal/passes/copyproto)

OWNER(
    g:go-library
    gzuykov
)

DATA(arcadia/library/go/yolint/internal/passes/copyproto/testdata)

TEST_CWD(library/go/yolint/internal/passes/copyproto)

INCLUDE(${ARCADIA_ROOT}/library/go/test/go_toolchain/recipe.inc)

END()
