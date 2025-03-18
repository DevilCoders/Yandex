UNITTEST_FOR(geobase/library)

BUILD_ONLY_IF(OS_LINUX AND NOT OS_ANDROID)

OWNER(
    g:geotargeting
)

CXXFLAGS(-DTESTDATA_VERSION=416)

SRCS(
    geobase/library/lookup_ut.cpp
)

PEERDIR(geobase/library/ut_helpers)

DATA(
    sbr://112446546 # 4.1.6
    arcadia/geobase/tests/test_data/data_416
)

END()
