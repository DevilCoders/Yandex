OWNER(g:cpp-contrib)

UNITTEST()

PEERDIR(
    ADDINCL library/cpp/xsltransform
    library/cpp/uri
)

SRCDIR(library/cpp/xsltransform)

SRCS(
    xsl_ut.cpp
)

END()
