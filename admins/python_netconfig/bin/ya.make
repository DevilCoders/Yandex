PY2_PROGRAM(netconfig)

PEERDIR(
    admins/python_netconfig/src
)

PY_SRCS(
    main.py
)

PY_MAIN(
    admins.python_netconfig.bin.main:main
)

END()
