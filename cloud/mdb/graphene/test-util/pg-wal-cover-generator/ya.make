PY3_PROGRAM(pg-wal-cover-generator)

STYLE_PYTHON()

OWNER(g:mdb)

PY_MAIN(main:main)

PEERDIR(
    cloud/mdb/graphene/test-util/common
)

PY_SRCS(
    TOP_LEVEL
    main.py
    scenarios.py
)

END()
