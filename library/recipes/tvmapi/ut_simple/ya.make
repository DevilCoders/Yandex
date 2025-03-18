PY3TEST()

OWNER(g:passport_infra)

TEST_SRCS(test.py)

PEERDIR(
    library/python/deprecated/ticket_parser2
)

# common usage
INCLUDE(${ARCADIA_ROOT}/library/recipes/tvmapi/recipe.inc)

END()
