OWNER(g:cpp-contrib)

LIBRARY()

PEERDIR(
    contrib/libs/libxslt
    contrib/libs/libexslt
    library/cpp/digest/lower_case
)

SRCS(
    xsltransform.cpp
    catxml.cpp
    datefun.cpp
)

END()
