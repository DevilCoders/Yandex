PY3_PROGRAM()

OWNER(g:ci)

PY_MAIN(ci.internal.tools.py.download_configs.main)

PY_SRCS(
    main.py
)

PEERDIR(
    library/python/startrek_python_client
)

END()
