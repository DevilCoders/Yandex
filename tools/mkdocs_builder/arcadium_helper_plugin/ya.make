PY23_LIBRARY()

OWNER(
    g:yatool
    workfork
)

PY_SRCS(
    __init__.py
)

PEERDIR(
    contrib/python/mkdocs
)

RESOURCE_FILES(
    PREFIX mkdocs_plugin/
    arcadium_helper.dist-info/entry_points.txt
    arcadium_helper.dist-info/METADATA
)

END()
