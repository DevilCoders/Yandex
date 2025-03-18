LIBRARY()

OWNER(stanly)

SRCS(
    tags.gperf
    attrs.h
    lextype.h
    tags.h
    tagstrings.cpp
)

GENERATE_ENUM_SERIALIZATION(lextype.h)

END()
