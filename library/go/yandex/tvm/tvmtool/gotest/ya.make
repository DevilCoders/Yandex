GO_TEST_FOR(library/go/yandex/tvm/tvmtool)

OWNER(
    buglloc
    g:passport_infra
    g:go-library
)

SIZE(MEDIUM)

DEFAULT(
    USE_TVM_TOOL
    0
)

# tvmtool recipe exists only for linux & darwin

IF (OS_LINUX)
    SET(
        USE_TVM_TOOL
        1
    )
ELSEIF(OS_DARWIN)
    SET(
        USE_TVM_TOOL
        1
    )
ENDIF()

IF (USE_TVM_TOOL)
    INCLUDE(${ARCADIA_ROOT}/library/recipes/tvmtool/recipe.inc)

    USE_RECIPE(
        library/recipes/tvmtool/tvmtool
        library/go/yandex/tvm/tvmtool/gotest/tvmtool.conf.json
    )
ENDIF()

END()
