GO_TEST_FOR(library/go/yolint/internal/passes/structtagcase)

OWNER(
    g:library-go
    gzuykov
)

DATA(arcadia/library/go/yolint/internal/passes/structtagcase/testdata)

TEST_CWD(library/go/yolint/internal/passes/structtagcase)

INCLUDE(${ARCADIA_ROOT}/library/go/test/go_toolchain/recipe.inc)

END()
