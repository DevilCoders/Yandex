PY3TEST()

OWNER(
    g:yatool
    workfork
)

TEST_SRCS(
    test_builder.py
)

DEPENDS(
    tools/mkdocs_builder
    tools/mkdocs_builder/tests/py2_bin
)

DATA(
    arcadia/devtools/ymake/tests/docsbuild/data
    arcadia/devtools/dummy_arcadia/test_java_coverage/src/consoleecho/CrazyCalculator.java
    arcadia/devtools/dummy_arcadia/test_java_coverage/ya.make
    arcadia/tools/mkdocs_builder/tests/data
)

PEERDIR(
    library/python/fs
)

END()

RECURSE(py2_bin)
