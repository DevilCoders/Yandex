OWNER(g:cpp-contrib)

LIBRARY()

SRCS(
    dict_reader.cpp
    dict_writer.cpp
    common.cpp
)

PEERDIR(
    library/cpp/containers/comptrie
    library/cpp/on_disk/chunks
)

END()
