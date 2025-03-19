LIBRARY()

OWNER(g:cs_dev)

SRCS(
    script.cpp
    task.cpp
    connection.cpp
)

PEERDIR(
    library/cpp/digest/md5
    library/cpp/graph
    library/cpp/logger/global
)

END()
