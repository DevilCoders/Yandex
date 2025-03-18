PY2_PROGRAM()

OWNER(okats)

PEERDIR(
    yp/python
    contrib/python/ipython
)

PY_SRCS(
    MAIN main.py
    dns.py
    dns2.py
)

END()
