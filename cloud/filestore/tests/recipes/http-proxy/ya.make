OWNER(g:cloud-nbs)

IF (SANITIZER_TYPE)
    PACKAGE()

    FROM_SANDBOX(3314775237
        EXECUTABLE OUT http-proxy-recipe
    )

    END()
ELSE()
    PY3_PROGRAM(http-proxy-recipe)

    PY_SRCS(__main__.py)

    PEERDIR(
        cloud/filestore/tests/python/lib

        library/python/testing/recipe
        library/python/testing/yatest_common
    )

    DEPENDS(
        cloud/filestore/tools/http_proxy
    )

    END()
ENDIF()
