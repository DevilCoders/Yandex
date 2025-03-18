PY3_PROGRAM(money_vh)

OWNER(g:antiadblock)

RESOURCE(
    yql_templ/vh.sql vh
    yql_templ/vh_map.sql vh_map
)

PY_SRCS(
    __main__.py
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
