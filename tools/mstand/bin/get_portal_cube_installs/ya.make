OWNER(
    g:mstand
)

PY3_PROGRAM()

PY_SRCS(
   __main__.py
)

PEERDIR(
    tools/mstand/experiment_pool
    quality/yaqlib/yaqutils

    library/python/nirvana
    search/gta/utils/statutils
    statbox/nile
    statbox/qb2_core
    yql/library/python
)

END()
