OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PEERDIR(
    library/python/testing/recipe
    library/python/testing/yatest_common
)

PY_SRCS(
    NAMESPACE dbaas_internal_api_image_recipe_lib
    __init__.py
)

END()
