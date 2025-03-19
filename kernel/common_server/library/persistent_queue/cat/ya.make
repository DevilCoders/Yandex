PROGRAM(pq_cat)

OWNER (g:cs_dev)

PEERDIR(
    kernel/common_server/library/persistent_queue/cat/proto
    kernel/common_server/library/persistent_queue/abstract
    kernel/common_server/library/persistent_queue/fake
    kernel/common_server/library/persistent_queue/kafka
    kernel/common_server/library/persistent_queue/logbroker
    library/cpp/getoptpb
    library/cpp/logger/global
    library/cpp/yconf/patcher
)

SRCS(
    main.cpp
)

END()
