PY3_PROGRAM(morda_awaps)

OWNER(g:antiadblock)

PY_SRCS(
    __main__.py
)

PEERDIR(
    library/python/resource
    library/python/yt
    yql/library/python
    contrib/python/pandas
    library/python/statface_client
    contrib/python/Jinja2
    antiadblock/tasks/tools
)

RESOURCE(
    yql_templ/morda_awaps.sql morda_awaps.sql
)

END()
