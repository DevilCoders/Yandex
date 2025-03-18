OWNER(
    g:maps-dragon-fighters
)

PY3_PROGRAM(gdal_proj_data)

PEERDIR(
    library/python/testing/recipe
    library/python/testing/yatest_common
    library/recipes/common
)

PY_SRCS(
    MAIN main.py
)

END()

RECURSE(
    tests
)
