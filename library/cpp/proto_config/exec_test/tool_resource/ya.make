PROGRAM()

OWNER(
    avitella
    mvel
    g:iss
)

PEERDIR(
    library/cpp/proto_config
)

SRCS(
    config.proto
    main.cpp
)

RESOURCE(
    config_resource.json /config/config_resource.json
    config.pb.txt        /config/config.pb.txt
    config2.pb.txt       /config/config2.pb.txt
)

END()
