PY3_PROGRAM(regex_stat_retriever)

OWNER(g:antiadblock)

RESOURCE(
    regex_stat_retriever_query.sql regex_stat_retriever_query
)

PY_SRCS(
    __main__.py
)

PEERDIR(
    library/python/resource
    contrib/python/pandas
    contrib/python/Jinja2
    yql/library/python
    antiadblock/tasks/tools
)

END()
