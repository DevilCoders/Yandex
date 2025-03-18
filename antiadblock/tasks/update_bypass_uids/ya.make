PY3_PROGRAM(update_bypass_uids)

OWNER(g:antiadblock)

PY_SRCS(
    __main__.py
)

RESOURCE(
    yql_templ/no_adblock_domains.sql no_adblock_domains.sql
    yql_templ/no_adblock_uids.sql no_adblock_uids.sql
)

PEERDIR(
    library/python/resource
    library/python/yt
    yql/library/python
    contrib/python/Jinja2
    antiadblock/tasks/tools
)

END()
