GO_TEST_FOR(library/go/yolint/internal/passes/exhaustivestruct)

OWNER(
    g:strm-admin
    grihabor
)

DATA(arcadia/library/go/yolint/internal/passes/exhaustivestruct/testdata)

TEST_CWD(library/go/yolint/internal/passes/exhaustivestruct)

INCLUDE(${ARCADIA_ROOT}/library/go/test/go_toolchain/recipe.inc)

END()
