PY3_PROGRAM(games_money)

OWNER(g:antiadblock)

PY_SRCS(
    __main__.py
)

RESOURCE(
    yql_templ/games_money.sql games_money
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
