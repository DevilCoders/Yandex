PY23_LIBRARY()

OWNER(
    g:geotargeting
    g:middle
)

PY_SRCS(
    __init__.py
    geolocation.pyx
)

PEERDIR(
    library/cpp/geolocation
)

END()
