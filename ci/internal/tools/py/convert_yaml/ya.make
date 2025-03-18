PY3_PROGRAM()

OWNER(g:ci)

PY_MAIN(ci.internal.tools.py.convert_yaml.main)

PY_SRCS(
    main.py
)

PEERDIR(
    contrib/python/ruamel.yaml
)

END()
