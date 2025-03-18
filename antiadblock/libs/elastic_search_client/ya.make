PY23_LIBRARY(elastic_search_client)

OWNER(g:antiadblock)

PY_SRCS(
    client.py
)

PEERDIR(
    contrib/python/requests
    contrib/python/retry
    antiadblock/libs/deploy_unit_resolver
)

END()
