LIBRARY()

OWNER(g:morphology)

PEERDIR(
    kernel/search_types
    library/cpp/enumbitset
    library/cpp/langmask
    library/cpp/token
    library/cpp/unicode/normalization
    library/cpp/langs
)

SRCS(
    abc.h
    alphaux.h
    alphabet.cpp
    directory.cpp
    normalizer.cpp
)

SET(
    output_includes
    kernel/lemmer/alpha/abc.h
    kernel/lemmer/alpha/default_converters.h
    kernel/lemmer/alpha/directory.h
    util/generic/singleton.h
    library/cpp/langs/langs.h
    library/cpp/token/charfilter.h
    util/charset/wide.h
    util/generic/singleton.h
)
PYTHON(
    build_abc_data.py abc_data.cpp
    IN abc_code.py
    IN ru_normalizer.cpp.tpl
    OUT abc_data.cpp
    OUTPUT_INCLUDES ${output_includes}
)

END()

# vim: set ft=cmake:
