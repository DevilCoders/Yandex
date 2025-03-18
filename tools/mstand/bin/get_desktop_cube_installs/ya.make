OWNER(
    g:mstand-online
)

PY3_PROGRAM()

PY_SRCS(
   __main__.py
)

PEERDIR(
    tools/mstand/experiment_pool
    quality/yaqlib/yaqutils

    metrika/uatraits/python
    statbox/nile
    statbox/qb2
)

END()
