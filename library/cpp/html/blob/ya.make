LIBRARY()

OWNER(stanly)

SRCS(
    document.proto
    chunks.cpp
    document.cpp
    internal.cpp
    output.cpp
    pack.cpp
)

PEERDIR(
    library/cpp/html/escape
    library/cpp/html/face
    library/cpp/html/spec
    library/cpp/packedtypes
)

END()
