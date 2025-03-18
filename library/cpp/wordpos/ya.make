LIBRARY()

OWNER(stanly)

SRCS(
    superlong_hit.cpp
    wordpos.cpp
    wordpos_out.cpp
)

GENERATE_ENUM_SERIALIZATION(wordpos.h)

END()
