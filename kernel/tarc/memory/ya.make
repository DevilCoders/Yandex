OWNER(g:cpp-contrib)

LIBRARY()

SRCS(
    archive.cpp
    archive_buffer.cpp
    archive_file.cpp
    archive_parser.cpp
)

PEERDIR(
    kernel/tarc/disk
    kernel/tarc/markup_zones
)

END()
