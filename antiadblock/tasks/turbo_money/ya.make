PY3_PROGRAM(turbo_money)

OWNER(g:antiadblock)

PY_SRCS(
    __main__.py
)

RESOURCE(
    yql_templ/turbo.sql turbo
    yql_templ/turbo_pageid_impid.sql turbo_pageid_impid
    yql_templ/aab_only_turbo_pages.sql aab_only_turbo_pages
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
