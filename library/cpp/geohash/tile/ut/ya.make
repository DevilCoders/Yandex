UNITTEST_FOR(library/cpp/geohash/tile)

OWNER(
    g:geotargeting
)

PEERDIR(
    contrib/libs/geos
)

SRCS(
    tile_ut.cpp
)

DATA(
    sbr://1117955985 # unusual_geometries.txt
)

SIZE(MEDIUM)

END()
