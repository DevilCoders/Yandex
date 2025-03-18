LIBRARY()

OWNER(g:unstructured-data)

SRCS(
    domtree.cpp
    numhandler.cpp
    builder.cpp
)

PEERDIR(
    library/cpp/charset
    library/cpp/html/html5
    library/cpp/html/spec
    library/cpp/html/face
    library/cpp/wordpos
    library/cpp/numerator
)

END()
