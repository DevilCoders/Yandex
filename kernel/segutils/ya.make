OWNER(g:cpp-contrib)

LIBRARY()

SRCS(
    numerator_utils.cpp
)

PEERDIR(
    kernel/segnumerator
    kernel/segnumerator/utils
    library/cpp/deprecated/dater_old
    library/cpp/html/pdoc
    library/cpp/mime/types
    kernel/hosts/owner
    kernel/recshell
)

END()
