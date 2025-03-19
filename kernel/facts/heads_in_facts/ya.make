LIBRARY()

OWNER(
    g:facts
)

PEERDIR(
    kernel/facts/dynamic_list_replacer
    library/cpp/string_utils/base64
    kernel/searchlog
)

SRCS(
    heads_in_facts.cpp
)

END()
