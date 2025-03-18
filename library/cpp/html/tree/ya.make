OWNER(g:cpp-contrib)

LIBRARY()

PEERDIR(
    contrib/libs/libxml
    library/cpp/charset
    library/cpp/html/entity
    library/cpp/html/face
    library/cpp/html/storage
    library/cpp/numerator
    library/cpp/xml/doc
    library/cpp/xml/document
)

SRCS(
    tree.cpp
    build.cpp
    buildxml.cpp
    buildxml_xpath.cpp
    out.cpp
    xmltree.cpp
)

END()
