UNITTEST()

OWNER(g:snippets)

SRCS(
    char_class_ut.cpp
    clearchar_ut.cpp
    hilited_length_ut.cpp
    pixel_length_ut.cpp
    smartcut_ut.cpp
)

PEERDIR(
    kernel/qtree/richrequest
    kernel/snippets/smartcut
    kernel/snippets/strhl
)

END()
