PY3_LIBRARY()
OWNER(g:antiadblock)

PY_SRCS(
    emulator.py
)

PEERDIR(
    contrib/python/pandas
    adv/pcode/zfp/juggler_emulator/lib
)

END()
