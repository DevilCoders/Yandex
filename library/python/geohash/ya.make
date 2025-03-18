PY23_LIBRARY()

OWNER(
    mstebelev
    g:geotargeting
)

PY_SRCS(
    __init__.py
    geohash.pyx
)

PEERDIR(
    library/cpp/geohash
    library/cpp/geohash/tile
)

END()
