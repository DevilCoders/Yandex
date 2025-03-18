PY3_PROGRAM(morda_aab_cookies)

OWNER(lexx-evd)

PY_SRCS(
    MAIN __main__.py
)

RESOURCE(
    yql_templ/cookies.sql cookies.sql
)

PEERDIR(
    library/python/resource
    contrib/python/Jinja2
    yql/library/python
    contrib/python/pandas
    contrib/python/click
    antiadblock/tasks/tools
)

END()
