PY2_PROGRAM()

OWNER(glebx777)

PY_SRCS(
    TOP_LEVEL main.py
)

PY_MAIN(main:main)

PEERDIR(
    infra/yp_quota_distributor/lib
)

NO_CHECK_IMPORTS()

END()
