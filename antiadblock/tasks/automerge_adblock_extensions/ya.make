PY3_PROGRAM(automerge_adblock_extensions)

OWNER(g:antiadblock)

PY_SRCS(
    __main__.py
)

PEERDIR(
    antiadblock/tasks/tools
)

END()
