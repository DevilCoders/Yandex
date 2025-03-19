PY3_PROGRAM(vr_gen)

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    contrib/python/humanfriendly
    contrib/python/pyaml
)

PY_SRCS(vr_gen.py)

PY_MAIN(cloud.mdb.tools.vr_gen.vr_gen:main)

END()
