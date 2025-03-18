PY2_PROGRAM(generate_interfaces.py)

OWNER(
    g:marketsre
)

PEERDIR(
    tools/python-netconfig-static/src
)

PY_SRCS(
    main.py
)

PY_MAIN(
    tools.python-netconfig-static.bin.generate_interfaces.main:main
)

END()
