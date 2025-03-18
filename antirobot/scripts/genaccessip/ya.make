OWNER(g:antirobot)

PY23_LIBRARY()

PY_SRCS(
    genaccessip.py
)

PEERDIR(
    contrib/python/certifi
    contrib/python/dnspython
    contrib/python/urllib3
)

END()
