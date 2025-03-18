OWNER(g:antiadblock)

PY2_PROGRAM(configs_api_bin)

PY_SRCS(
    MAIN wsgi.py
    config_gunicorn.py
)

PEERDIR(
    contrib/python/gunicorn
    antiadblock/configs_api/lib
)

END()
