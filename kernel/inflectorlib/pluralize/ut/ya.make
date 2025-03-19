UNITTEST()

OWNER(
    alzobnin
    gotmanov

)

PEERDIR(
    library/cpp/langs
    ADDINCL kernel/inflectorlib/pluralize
)

SRCDIR(kernel/inflectorlib/pluralize)

SRCS(
    pluralize_ut.cpp
)

END()
