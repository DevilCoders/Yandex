GO_TEST_FOR(library/go/yolint/internal/passes/scopelint)

OWNER(
    g:go-library
    gzuykov
)

DATA(arcadia/library/go/yolint/internal/passes/scopelint/testdata)

TEST_CWD(library/go/yolint/internal/passes/scopelint)

INCLUDE(${ARCADIA_ROOT}/library/go/test/go_toolchain/recipe.inc)

END()
