PY2_PROGRAM(change_current_cookie)

OWNER(g:antiadblock)

PY_SRCS(
    __main__.py
)

PEERDIR(
    contrib/python/enum34
    library/python/tvmauth
    library/python/startrek_python_client
    antiadblock/libs/utils
    antiadblock/tasks/tools
    antiadblock/adblock_rule_sonar/sonar/lib
)

END()
