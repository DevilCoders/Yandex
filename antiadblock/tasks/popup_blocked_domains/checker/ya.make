PY3_PROGRAM(popup_blocked_domains_extractor_checker)

OWNER(g:antiadblock)

RESOURCE(
    popup_blocked_domains_extractor_checker.sql popup_blocked_domains_extractor_checker
)

PY_SRCS(
    __main__.py
)

PEERDIR(
    library/python/resource
    contrib/python/pandas
    yql/library/python
    antiadblock/tasks/tools
)

END()
