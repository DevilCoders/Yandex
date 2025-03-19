PY3_PROGRAM(vr_gen2)

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    cloud/mdb/tools/vr_gen2/internal
)

PY_SRCS(
    vr_gen2.py
    logs.py
)

PY_MAIN(cloud.mdb.tools.vr_gen2.bin.vr_gen2:main)

END()
