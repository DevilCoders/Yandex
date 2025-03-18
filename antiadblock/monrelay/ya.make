PY2_PROGRAM(monrelay)

OWNER(g:antiadblock)

PY_SRCS(
    __init__.py
    __main__.py
    jslogger.py
    data_sources/__init__.py
    data_sources/solomon.py
    data_sources/dsyql.py
    data_sources/stat.py
)

PEERDIR(
    library/python/tvmauth
    yt/python/client
    library/python/yt
    yql/library/python
    library/python/statface_client
)

END()
