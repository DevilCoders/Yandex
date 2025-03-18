LIBRARY()

OWNER(
    velavokr
    ulyanov
    g:antimalware
    g:antiwebspam
)

PEERDIR(
    kernel/hosts/owner
    library/cpp/binsaver
    library/cpp/containers/comptrie
    library/cpp/string_utils/quote
    library/cpp/string_utils/url
    library/cpp/uri
)

SRCS(
    infected_masks.cpp
    masks_comptrie.cpp
    sb_masks.cpp
)

GENERATE_ENUM_SERIALIZATION(infected_masks.h)

END()
