PY3_PROGRAM(morda_awaps_by_templates)

OWNER(g:antiadblock)

RESOURCE(
    yql_templ/awaps_templates.sql awaps_templates
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
