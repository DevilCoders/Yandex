PY3_PROGRAM(bin)

OWNER(g:antiadblock)

PY_SRCS(
    MAIN __main__.py
)

PEERDIR(
    contrib/python/click
    contrib/python/pandas
    library/python/resource
    library/python/yt
    yql/library/python
    contrib/python/Jinja2
    library/python/statface_client
    antiadblock/tasks/tools
    antiadblock/tasks/money_by_service_id/lib
)

RESOURCE(
    yql_templ/pageids.sql pageids
    yql_templ/bschevent.sql bschevent
    yql_templ/bschevent_only_blocks.sql bschevent_only_blocks
    yql_templ/eventbad.sql eventbad
)

END()
