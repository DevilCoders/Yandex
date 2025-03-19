PY3_LIBRARY()

OWNER(music-sre)

PY_SRCS(
    NAMESPACE src
    solomon_mr/Config.py
    solomon_mr/DataPoint.py
    solomon_mr/Juggler.py
    solomon_mr/SolomonDAO.py
    solomon_mr/SolomonMR.py
)

RESOURCE(
    logging.yaml logging.yaml
)

PEERDIR(
    contrib/python/PyYAML
    contrib/python/attrs
    contrib/python/kazoo
    contrib/python/requests
    library/python/resource
)

END()
