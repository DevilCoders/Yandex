GO_TEST_FOR(library/go/yolint/internal/middlewares)

OWNER(g:go-library)

DATA(arcadia/library/go/yolint/internal/middlewares/testdata)

TEST_CWD(library/go/yolint/internal/middlewares)

INCLUDE(${ARCADIA_ROOT}/library/go/test/go_toolchain/recipe.inc)

END()
