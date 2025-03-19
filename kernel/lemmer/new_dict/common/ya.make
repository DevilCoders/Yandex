LIBRARY()

OWNER(g:morphology)

SRCS(
    custom_language.cpp
    new_language.cpp
)

PEERDIR(
    kernel/search_types
    kernel/lemmer/dictlib
    kernel/lemmer/new_engine
    kernel/lemmer/versions
)

END()
