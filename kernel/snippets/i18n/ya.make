LIBRARY()

OWNER(g:snippets)

SRCS(
    i18n.cpp
)

PEERDIR(
    contrib/libs/i18n
    library/cpp/archive
    library/cpp/langs
)

ARCHIVE(
    NAME snippets_locales.inc
    blr.mo
    eng.mo
    ind.mo
    kaz.mo
    rus.mo
    tur.mo
    ukr.mo
)

END()
