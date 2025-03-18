PY3_PROGRAM()

OWNER(borman)

PEERDIR(
    library/python/luigi
)

PY_MAIN(library.python.luigi.static_server:luigid_static)

END()
