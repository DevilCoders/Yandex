PY3_PROGRAM(popup_blocked_domains_extractor)

OWNER(g:antiadblock)

RESOURCE(
    popup_blocked_domains_extractor.sql popup_blocked_domains_extractor
)

PY_SRCS(
    __main__.py
)

PEERDIR(
    library/python/resource
    contrib/python/Jinja2
    contrib/python/pandas
    yql/library/python
    antiadblock/tasks/tools
)

END()
