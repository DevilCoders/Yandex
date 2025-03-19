OWNER(g:cpp-contrib)

LIBRARY()

SRCS(
    translit.cpp
    transmap.cpp
    symbolmaps.cpp
)

PEERDIR(
    kernel/lemmer
    kernel/lemmer/untranslit
    library/cpp/langs
    library/cpp/token
)

END()
