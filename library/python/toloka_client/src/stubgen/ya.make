OWNER(losev g:toloka-analytics)

PY3_LIBRARY()
LICENSE(Apache-2.0)
VERSION(0.0.1)
PY_SRCS(
    NAMESPACE stubgen
    viewers/markdown_viewer.py
    __init__.py
)
PEERDIR(
    contrib/python/docstring-parser
    library/python/stubmaker/src
)
END()
