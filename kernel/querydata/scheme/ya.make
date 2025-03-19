LIBRARY()

OWNER(
    alexbykov
    mvel
    velavokr
)

SRCS(
    qd_scheme.cpp
    qd_scheme_v1.cpp
    qd_scheme_v2.cpp
)

PEERDIR(
    library/cpp/scheme
    kernel/querydata/common
    kernel/querydata/idl
)

END()
