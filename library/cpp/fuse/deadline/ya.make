LIBRARY()

OWNER(g:arc)

IF (NOT MSVC)
    CXXFLAGS(-Wimplicit-fallthrough)
ENDIF()

SRCS(deadline.cpp)

PEERDIR(
    library/cpp/containers/intrusive_rb_tree
    library/cpp/threading/future
)

END()
