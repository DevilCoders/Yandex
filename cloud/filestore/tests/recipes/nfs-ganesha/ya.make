OWNER(g:cloud-nbs)

IF (SANITIZER_TYPE)
    PACKAGE()

    FROM_SANDBOX(3314775237
        EXECUTABLE OUT nfs-ganesha-recipe
    )

    END()
ELSE()
    PY3_PROGRAM(nfs-ganesha-recipe)

    PY_SRCS(__main__.py)

    PEERDIR(
        cloud/filestore/tests/python/lib

        library/python/testing/recipe
        library/python/testing/yatest_common
    )

    END()
ENDIF()
