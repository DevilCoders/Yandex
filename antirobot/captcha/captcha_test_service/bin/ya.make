PY3_PROGRAM(captcha_test_server)

OWNER(g:antirobot)

PY_SRCS(
    MAIN server.py
    logic.py
)

PEERDIR(
    contrib/python/numpy
    contrib/python/requests
    contrib/python/scipy
    contrib/python/opencv-python
    contrib/python/Flask
    contrib/python/Flask-WTF
    contrib/python/Pillow
    library/python/deprecated/ticket_parser2
)

END()
