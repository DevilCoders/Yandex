UNITTEST()

PEERDIR(
    ADDINCL tools/clustermaster/common
    ADDINCL tools/clustermaster/master/lib
    ADDINCL tools/clustermaster/worker/lib
    library/cpp/archive
)

SRCS(
    data.cpp
    speed_test.cpp
)

ARCHIVE(
    NAME test_data.inc
    500x500.sh
    param_mapping.sh
)

OWNER(
    g:clustermaster
)

END()
