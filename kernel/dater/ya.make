LIBRARY()

SET(
    RAGEL6_FLAGS
    -l
    -F1
)

OWNER(velavokr)

NO_JOIN_SRC()

SRCS(
    patterns_dig.rl6
    patterns_prescan.rl6
    chunk.cpp
    dater.cpp
    dater_simple.cpp
    eval_chunk.cpp
    pattern_traits.cpp
    scanner.cpp
    text_concatenator.cpp
)

GENERATE_ENUM_SERIALIZATION(pattern_traits.h)

SET(
    _const_deps
    symbols.rl6
    pat_common.rl6
)

SOURCE_GROUP("Custom Builds" FILES ${CURDIR}/patterns_wrd.rl6.template)

RUN_LUA(
    preprocessor.lua ${ARCADIA_ROOT} patterns_wrd.rl6.template patterns_wrd_by.rl6
        datafile=regdata/data_by.rl6 scannerlang=LANG_BEL
    IN regdata/data_by.rl6 patterns_wrd.rl6.template ${_const_deps}
    OUT patterns_wrd_by.rl6
    OUTPUT_INCLUDES ${ARCADIA_ROOT}/kernel/dater/scanner.h
)

RUN_LUA(
    preprocessor.lua ${ARCADIA_ROOT} patterns_wrd.rl6.template patterns_wrd_ca.rl6
        datafile=regdata/data_ca.rl6 scannerlang=LANG_CAT
    IN regdata/data_ca.rl6 patterns_wrd.rl6.template ${_const_deps}
    OUT patterns_wrd_ca.rl6
    OUTPUT_INCLUDES ${ARCADIA_ROOT}/kernel/dater/scanner.h
)

RUN_LUA(
    preprocessor.lua ${ARCADIA_ROOT} patterns_wrd.rl6.template patterns_wrd_cz.rl6
        datafile=regdata/data_cz.rl6 scannerlang=LANG_CZE
    IN regdata/data_cz.rl6 patterns_wrd.rl6.template ${_const_deps}
    OUT patterns_wrd_cz.rl6
    OUTPUT_INCLUDES ${ARCADIA_ROOT}/kernel/dater/scanner.h
)

RUN_LUA(
    preprocessor.lua ${ARCADIA_ROOT} patterns_wrd.rl6.template patterns_wrd_de.rl6
        datafile=regdata/data_de.rl6 scannerlang=LANG_GER
    IN regdata/data_de.rl6 patterns_wrd.rl6.template ${_const_deps}
    OUT patterns_wrd_de.rl6
    OUTPUT_INCLUDES ${ARCADIA_ROOT}/kernel/dater/scanner.h
)

RUN_LUA(
    preprocessor.lua ${ARCADIA_ROOT} patterns_wrd.rl6.template patterns_wrd_en.rl6
        datafile=regdata/data_en.rl6 scannerlang=LANG_ENG
    IN regdata/data_en.rl6 patterns_wrd.rl6.template ${_const_deps}
    OUT patterns_wrd_en.rl6
    OUTPUT_INCLUDES ${ARCADIA_ROOT}/kernel/dater/scanner.h
)

RUN_LUA(
    preprocessor.lua ${ARCADIA_ROOT} patterns_wrd.rl6.template patterns_wrd_es.rl6
        datafile=regdata/data_es.rl6 scannerlang=LANG_SPA
    IN regdata/data_es.rl6 patterns_wrd.rl6.template ${_const_deps}
    OUT patterns_wrd_es.rl6
    OUTPUT_INCLUDES ${ARCADIA_ROOT}/kernel/dater/scanner.h
)

RUN_LUA(
    preprocessor.lua ${ARCADIA_ROOT} patterns_wrd.rl6.template patterns_wrd_fr.rl6
        datafile=regdata/data_fr.rl6 scannerlang=LANG_FRE
    IN regdata/data_fr.rl6 patterns_wrd.rl6.template ${_const_deps}
    OUT patterns_wrd_fr.rl6
    OUTPUT_INCLUDES ${ARCADIA_ROOT}/kernel/dater/scanner.h
)

RUN_LUA(
    preprocessor.lua ${ARCADIA_ROOT} patterns_wrd.rl6.template patterns_wrd_it.rl6
        datafile=regdata/data_it.rl6 scannerlang=LANG_ITA
    IN regdata/data_it.rl6 patterns_wrd.rl6.template ${_const_deps}
    OUT patterns_wrd_it.rl6
    OUTPUT_INCLUDES ${ARCADIA_ROOT}/kernel/dater/scanner.h
)

RUN_LUA(
    preprocessor.lua ${ARCADIA_ROOT} patterns_wrd.rl6.template patterns_wrd_kz.rl6
        datafile=regdata/data_kz.rl6 scannerlang=LANG_KAZ
    IN regdata/data_kz.rl6 patterns_wrd.rl6.template ${_const_deps}
    OUT patterns_wrd_kz.rl6
    OUTPUT_INCLUDES ${ARCADIA_ROOT}/kernel/dater/scanner.h
)

RUN_LUA(
    preprocessor.lua ${ARCADIA_ROOT} patterns_wrd.rl6.template patterns_wrd_pl.rl6
        datafile=regdata/data_pl.rl6 scannerlang=LANG_POL
    IN regdata/data_pl.rl6 patterns_wrd.rl6.template ${_const_deps}
    OUT patterns_wrd_pl.rl6
    OUTPUT_INCLUDES ${ARCADIA_ROOT}/kernel/dater/scanner.h
)

RUN_LUA(
    preprocessor.lua ${ARCADIA_ROOT} patterns_wrd.rl6.template patterns_wrd_ru.rl6
        datafile=regdata/data_ru.rl6 scannerlang=LANG_RUS
    IN regdata/data_ru.rl6 patterns_wrd.rl6.template ${_const_deps}
    OUT patterns_wrd_ru.rl6
    OUTPUT_INCLUDES ${ARCADIA_ROOT}/kernel/dater/scanner.h
)

RUN_LUA(
    preprocessor.lua ${ARCADIA_ROOT} patterns_wrd.rl6.template patterns_wrd_tr.rl6
        datafile=regdata/data_tr.rl6 scannerlang=LANG_TUR
    IN regdata/data_tr.rl6 patterns_wrd.rl6.template ${_const_deps}
    OUT patterns_wrd_tr.rl6
    OUTPUT_INCLUDES ${ARCADIA_ROOT}/kernel/dater/scanner.h
)

RUN_LUA(
    preprocessor.lua ${ARCADIA_ROOT} patterns_wrd.rl6.template patterns_wrd_ua.rl6
        datafile=regdata/data_ua.rl6 scannerlang=LANG_UKR
    IN regdata/data_ua.rl6 patterns_wrd.rl6.template ${_const_deps}
    OUT patterns_wrd_ua.rl6
    OUTPUT_INCLUDES ${ARCADIA_ROOT}/kernel/dater/scanner.h
)

PEERDIR(
    kernel/segmentator/structs
    kernel/segnumerator/utils
    library/cpp/langs
    library/cpp/unicode/folding
    util/draft
)

END()
