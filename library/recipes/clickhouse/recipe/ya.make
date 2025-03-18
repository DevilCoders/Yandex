IF (NOT OS_WINDOWS)

PY23_LIBRARY()

OWNER(g:testenv)

PY_SRCS(__init__.py)

PEERDIR(
    contrib/python/requests

    library/python/testing/recipe
    library/python/testing/yatest_common
    library/recipes/common
)

END()

ENDIF()
