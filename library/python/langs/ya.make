PY23_LIBRARY()

OWNER(
    g:antiinfra
)

PY_SRCS(
    NAMESPACE
    library.langs
    langs.swg
)

PEERDIR(
    library/cpp/langs
)

END()
