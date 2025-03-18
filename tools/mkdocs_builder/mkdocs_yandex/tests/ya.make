PY23_TEST()

OWNER(
    g:locdoc
    workfork
)

TEST_SRCS(
    integration.py
    note.py
    regex.py
    ut.py
)

FORK_TEST_FILES()

PEERDIR(
    contrib/python/mkdocs
    locdoc/doc_tools/mkdocs/mkdocs_doccenter/mkdocs_doccenter
    tools/mkdocs_builder/mkdocs_yandex
)

DEPENDS(
    tools/mkdocs_builder
    contrib/python/mkdocs/bin
)

DATA(
    arcadia/locdoc/doc_tools/mkdocs/test-doc
    arcadia/tools/mkdocs_builder/mkdocs_yandex/tests/data
)

END()
