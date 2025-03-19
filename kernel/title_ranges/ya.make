LIBRARY()

OWNER(gotmanov)

SRCS(
    calcer.cpp
)

RESOURCE(
    title_ranges.rules title_ranges_remorph_rules
)

PEERDIR(
    dict/recognize/queryrec
    kernel/remorph/core
    kernel/remorph/matcher
    kernel/remorph/text
    kernel/title_ranges/lib
    library/cpp/bit_io
    library/cpp/charset
    library/cpp/resource
    library/cpp/solve_ambig
    ysite/yandex/reqanalysis
)

END()
