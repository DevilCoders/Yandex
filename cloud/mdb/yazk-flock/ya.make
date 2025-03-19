PY3_PROGRAM(yazk-flock)

STYLE_PYTHON()

OWNER(g:mdb)

PY_MAIN(cloud.mdb.yazk-flock.main)

PY_SRCS(main.py)

PEERDIR(
    library/python/ylock
    contrib/python/click
    contrib/python/marshmallow-dataclass
)

END()
