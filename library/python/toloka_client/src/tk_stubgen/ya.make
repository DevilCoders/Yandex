OWNER(alexdrydew g:toloka-crowd-instruments)

PY3_LIBRARY()
PY_SRCS(
    __init__.py
    builder/__init__.py
    builder/definitions/__init__.py
    builder/definitions/expanded_function_def.py
    builder/tk_representations_tree_builder.py
    constants/__init__.py
    constants/markdowns.py
    constants/stubs.py
    makers/__init__.py
    makers/make_markdowns.py
    makers/make_stubs.py
    util.py
    viewers/__init__.py
    viewers/markdown_viewer.py
    viewers/stub_viewer.py
)
PEERDIR(
    contrib/python/pytest
    library/python/stubmaker
    library/python/toloka_client/src/stubgen
)
END()
