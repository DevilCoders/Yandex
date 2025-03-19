PY3_LIBRARY(bootstrap.api.helper)

OWNER(g:ycselfhost)

PEERDIR(
    contrib/python/PyYAML
    contrib/python/requests
)

PY_SRCS(
    NAMESPACE bootstrap.api.helper

    app.py
)

END()
