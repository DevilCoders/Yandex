LIBRARY()

OWNER(
    g:facts
)

SRCS(
    common.cpp
    snippet_metafactor_calculator.cpp
)

GENERATE_ENUM_SERIALIZATION(kernel/facts/snippet_metafactor_calculator/snippet_metafactor_calculator.h)

END()
