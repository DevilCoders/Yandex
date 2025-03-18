OWNER(
    dronimal
    g:contrib
    g:strm-admin
)

UNITTEST_FOR(nginx/modules/strm_packager)

SRCS(
    vod_uri_ut.cpp
    vod_description_ut.cpp
    cenc_cipher_ut.cpp
    evp_cipher_ut.cpp
    future_utils_ut.cpp
    order_manager_ut.cpp
    repeatable_future_ut.cpp
    source_moov_cache_data_ut.cpp
)

DATA(sbr://2286534605) #video_1_3e338618de2d7b089a52943675de4890.mp4
DATA(sbr://2286536600) #video_1_7931f6e5b751ab3a9d76f6003fb25e27.mp4
DATA(sbr://2286537908) #sample.mp4
DATA(sbr://2286539142) #source.mp4

DATA(arcadia/nginx/modules/strm_packager/src/ut/data)

END()
