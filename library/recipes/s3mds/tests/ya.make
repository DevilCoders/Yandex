OWNER(
    g:maps-dragon-fighters
    g:s3
)

PY3TEST()

PEERDIR(
    contrib/python/boto3
)

TEST_SRCS(
    boto_tests.py
)

INCLUDE(${ARCADIA_ROOT}/library/recipes/s3mds/recipe.inc)

END()
