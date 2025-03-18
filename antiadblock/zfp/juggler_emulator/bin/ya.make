PY3_PROGRAM(juggler_emulator)
OWNER(g:antiadblock)

PY_SRCS(
    __main__.py
)

PEERDIR(
    antiadblock/tasks/tools
    contrib/python/PyYAML
    adv/pcode/zfp/juggler_emulator/lib
    antiadblock/zfp/juggler_emulator/lib
)

END()
