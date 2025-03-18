OWNER(g:cpp-contrib)

LIBRARY()

PEERDIR(
    library/cpp/string_utils/base64
    library/cpp/digest/md5
    library/cpp/deprecated/atomic
)

SRCS(
    lcookie.cpp
    lkey_reader.cpp
)

END()
