PY3_PROGRAM(ext_captcha_demo)

OWNER(g:antirobot)

PY_SRCS(
    MAIN server.py
)

PEERDIR(
    contrib/python/requests
    contrib/python/Flask
    contrib/python/Flask-WTF
)

END()
