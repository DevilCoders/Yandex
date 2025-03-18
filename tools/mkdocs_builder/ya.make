OWNER(
    workfork
)

IF(MKDOCS_PYTHON2)  # see LOGOSWEB-351, DOCSTOOLS-18
    PY2_PROGRAM()
    PY_MAIN(tools.mkdocs_builder.lib.build)

    PEERDIR(
        tools/mkdocs_builder/lib
    )

    END()
ELSE()
    PY3_PROGRAM()
    PY_MAIN(tools.mkdocs_builder.lib.build)

    PEERDIR(
        tools/mkdocs_builder/lib
    )

    END()
ENDIF()



RECURSE_FOR_TESTS(tests)
