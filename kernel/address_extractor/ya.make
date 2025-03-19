LIBRARY()

OWNER(yurakura)

PEERDIR(
    kernel/geograph
    kernel/remorph/common
    kernel/remorph/facts
    kernel/remorph/text
    library/cpp/charset
    library/cpp/getopt/small
    library/cpp/langmask
)

SRCS(
    combination_builder.h
    options.h
    mine_address.h
    address_extractor.h
    options.cpp
    mine_address.cpp
    address_extractor.cpp
)

END()
