OWNER(
    lagrunge
    sobols
    suns
)

LIBRARY()

SRCS(
    unistat.cpp
    raii.cpp
)

GENERATE_ENUM_SERIALIZATION(types.h)

PEERDIR(
    library/cpp/json
    library/cpp/json/writer
    library/cpp/unistat/idl
    library/cpp/deprecated/atomic
)

END()
