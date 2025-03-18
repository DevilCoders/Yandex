OWNER(
    g:maps-dragon-fighters
)

PY3TEST()

PEERDIR(
    contrib/python/pyproj
    contrib/python/GDAL
)

TEST_SRCS(
    tests.py
)

INCLUDE(${ARCADIA_ROOT}/library/recipes/gdal_proj_data/recipe.inc)

END()
