PROGRAM(balancer_config_to_json)

OWNER(
    g:balancer
    kulikov
    mvel
)

SRCS(
    main.cpp
)

PEERDIR(
    library/cpp/lua
    library/cpp/config
)

END()
