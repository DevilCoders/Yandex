PY3_PROGRAM(generate-pillar-pgp)

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    contrib/python/PGPy
    cloud/mdb/scripts/pillar_generators/lockbox
)

PY_MAIN(cloud.mdb.scripts.pillar_generators.pgp:main)

ALL_PY_SRCS()

END()
