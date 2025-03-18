IF (NOT OS_WINDOWS)

PY2_PROGRAM(clickhouse-recipe)

OWNER(g:testenv)

PY_SRCS(__main__.py)

PEERDIR(
    library/recipes/clickhouse/recipe
    library/recipes/common
)

END()

RECURSE(
    recipe
)

ENDIF()
