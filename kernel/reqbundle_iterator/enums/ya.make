LIBRARY()

OWNER(
    g:base
    mvel
)

GENERATE_ENUM_SERIALIZATION(reqbundle_enums.h)

SRCS(
    reqbundle_enums.cpp
)

END()
