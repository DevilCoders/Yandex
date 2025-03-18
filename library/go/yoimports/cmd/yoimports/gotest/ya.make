GO_TEST_FOR(library/go/yoimports/cmd/yoimports)

OWNER(
    gzuykov
    buglloc
    g:go-library
)

DATA(arcadia/library/go/yoimports/cmd/yoimports/testdata)

TEST_CWD(library/go/yoimports/cmd/yoimports)

INCLUDE(${ARCADIA_ROOT}/library/go/test/go_toolchain/recipe.inc)

END()
