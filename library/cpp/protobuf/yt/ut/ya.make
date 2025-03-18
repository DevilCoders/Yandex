OWNER(stanly)

UNITTEST_FOR(library/cpp/protobuf/yt)

SRCS(
    main_ut.cpp
)

PEERDIR(
    library/cpp/protobuf/json
    library/cpp/protobuf/yt
    library/cpp/protobuf/yt/ut/proto
    library/cpp/resource
    library/cpp/yson/node
)

RESOURCE(
    library/cpp/protobuf/yt/ut/wifi_profile.json /wifi_profile.json
)

NEED_CHECK()

END()
