OWNER(g:cpp-contrib)

LIBRARY()

SRCS(
    numerator_utils.cpp
)

PEERDIR(
    kernel/dater/convert_old
    kernel/segmentator/structs
    kernel/segutils
    library/cpp/deprecated/dater_old
    library/cpp/deprecated/dater_old/scanner
)

END()
