PY2_PROGRAM(mkdocs_builder)

OWNER(
    workfork
)

PY_MAIN(tools.mkdocs_builder.lib.build)

PEERDIR(
    tools/mkdocs_builder/lib
)

END()
