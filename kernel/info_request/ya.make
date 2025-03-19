LIBRARY()

# Author: alexeykruglov@
OWNER(
    g:base
    mvel
)

SRCS(
    inforequestformatter.cpp
)

PEERDIR(
    library/cpp/json/writer
    library/cpp/html/pcdata
    kernel/factor_storage
    kernel/search_daemon_iface
)

END()
