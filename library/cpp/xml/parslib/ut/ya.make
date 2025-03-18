OWNER(g:cpp-contrib)

UNITTEST_FOR(library/cpp/xml/parslib)

PEERDIR(
    library/cpp/digest/old_crc
)

SRCS(
    fastlex_ut.cpp
    xmlsax_ut.cpp
)

END()
