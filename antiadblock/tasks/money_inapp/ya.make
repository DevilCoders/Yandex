PY3_PROGRAM(money_inapp)

OWNER(g:antiadblock)

PY_SRCS(
    __main__.py
)

RESOURCE(
    yql_templ/inapp.sql inapp
)

PEERDIR(
    library/python/resource
    contrib/python/Jinja2
    library/python/yt
    yql/library/python
    contrib/python/pandas
    library/python/statface_client
    antiadblock/tasks/tools
)

END()
