PY3_PROGRAM()
OWNER(g:antiadblock)

PY_SRCS(
    __main__.py
)

PEERDIR(
    library/python/startrek_python_client
    antiadblock/tasks/tools
    adv/pcode/zfp/diff_calculator/lib
)

END()
