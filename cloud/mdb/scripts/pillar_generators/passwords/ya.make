PY3_PROGRAM(generate-pillar-passwords)

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    contrib/python/pyaml
    cloud/mdb/scripts/pillar_generators/lockbox
)

PY_MAIN(cloud.mdb.scripts.pillar_generators.passwords:main)

ALL_PY_SRCS()

END()
