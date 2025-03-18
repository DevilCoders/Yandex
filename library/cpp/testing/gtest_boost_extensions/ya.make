LIBRARY()
OWNER(
    bulatman
    g:cpp-contrib
)

PEERDIR(
    contrib/restricted/boost
    contrib/restricted/googletest/googlemock
    contrib/restricted/googletest/googletest
)

SRCS(
    pretty_printers.cpp
)

END()

RECURSE_FOR_TESTS(ut)
