PY3_PROGRAM()

OWNER(g:ci)

PY_MAIN(ci.internal.tools.py.extract_boxes.main)

PY_SRCS(
    main.py
)

PEERDIR(
    contrib/python/ruamel.yaml
)

END()
