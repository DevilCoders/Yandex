PY2_PROGRAM(generate_hosts.py)

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
    tools.python-netconfig-static.bin.generate_hosts.main:main
)

END()
