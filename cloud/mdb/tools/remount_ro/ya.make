PY3_PROGRAM(remount_ro)

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    contrib/python/sh
)

PY_SRCS(remount_ro.py)

PY_MAIN(cloud.mdb.tools.remount_ro.remount_ro:main)

END()
