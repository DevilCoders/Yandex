LIBRARY()

OWNER(
    alexbykov
    mvel
    velavokr
)

GENERATE_ENUM_SERIALIZATION(qd_saas_key.h)

SRCS(
    qd_saas_key.cpp
    qd_saas_trie_key.cpp
)

END()
