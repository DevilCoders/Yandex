LIBRARY()

OWNER(g:yane)

PEERDIR(
    kernel/fio_extractor
    kernel/remorph/input
    kernel/remorph/facts
    library/cpp/charset
)

SRCS(
    gleiche.cpp
    fact_to_fio.cpp
)

END()
