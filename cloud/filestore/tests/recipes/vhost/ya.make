OWNER(g:cloud-nbs)

IF (SANITIZER_TYPE)
    PACKAGE()

    FROM_SANDBOX(3314775237
        EXECUTABLE OUT vhost-recipe
    )

    END()
ELSE()
    PY3_PROGRAM(vhost-recipe)

    PY_SRCS(__main__.py)

    PEERDIR(
        cloud/filestore/tests/python/lib

        library/python/testing/recipe
        library/python/testing/yatest_common
    )

    DEPENDS(
        cloud/filestore/vhost
    )

    END()
ENDIF()
