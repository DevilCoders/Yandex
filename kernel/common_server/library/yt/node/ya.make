LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/json
    library/cpp/yson/node
)

SRCS(
    cast.cpp
    json.cpp
)

END()
