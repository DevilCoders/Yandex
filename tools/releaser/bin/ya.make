PY3_PROGRAM(releaser)

OWNER(g:tools-python)

VERSION(0.78)

PEERDIR(
    tools/releaser/src
)

PY_MAIN(
    tools.releaser.src.cli.main:cli
)

NO_CHECK_IMPORTS(hjson.ordered_dict)  # там поддержка второго питона с кучей try вокруг импортов

END()
