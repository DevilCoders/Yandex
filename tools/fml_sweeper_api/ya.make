LIBRARY(fml_sweeper_api)

OWNER(serkh)

PY_SRCS(
    NAMESPACE fml_sweeper_api
    __init__.py
    exceptions.py
    client.py
    functions.py
    passes.py
    utils.py
)

PEERDIR(
    contrib/python/requests
)

END()

RECURSE(
    ut
)
