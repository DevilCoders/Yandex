OWNER(ilnurkh)

LIBRARY()

SRCS(
    file_with_header.cpp
)

PEERDIR(
    library/cpp/svnversion
)

END()
RECURSE_FOR_TESTS(ut)


