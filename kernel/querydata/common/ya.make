LIBRARY()

OWNER(
    alexbykov
    mvel
    velavokr
)

SRCS(
    qi_shardnum.h
    qd_key_token.cpp
    qd_source_names.cpp
    qd_util.cpp
    qd_valid.cpp
    querydata_traits.cpp
)

PEERDIR(
    kernel/querydata/idl
    util/draft
)

END()
