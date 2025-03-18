PY2_PROGRAM(generate_ammo_stub_table)

OWNER(g:antiadblock)

PY_SRCS(
    __main__.py
)

PEERDIR(
    yt/python/client
    library/python/yt
    contrib/python/tornado/tornado-4
    antiadblock/tasks/tools
)

END()
