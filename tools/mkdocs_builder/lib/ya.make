PY23_LIBRARY()

OWNER(
    workfork
)

PY_SRCS(
    build.py
    process.py
    util.py
)

PEERDIR(
    contrib/python/python-slugify
    contrib/python/Jinja2
    contrib/python/mkdocs
    library/python/fs

    # themes
    locdoc/doc_tools/mkdocs/mkdocs_doccenter/mkdocs_doccenter
    locdoc/doc_tools/mkdocs/mkdocs_daas/mkdocs_daas
    tools/mkdocs_builder/theme
    yql/tools/docs/theme

    # plugins
    logos/tools/mkdocs/plugin
    tools/mkdocs_builder/mkdocs_yandex
    tools/mkdocs_builder/arcadium_helper_plugin
    yql/library/python
)

IF (PYTHON2)
    PEERDIR(
        contrib/python/typing
    )
ENDIF()

END()
