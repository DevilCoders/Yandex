LIBRARY()

OWNER(
    g:geotargeting
)

PEERDIR(
    contrib/libs/geos
    library/cpp/geohash
)

SRCS(
    tile.cpp
)

END()
