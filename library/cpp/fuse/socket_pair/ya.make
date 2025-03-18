LIBRARY()

OWNER(g:arc)

IF (NOT MSVC)
    CXXFLAGS(-Wimplicit-fallthrough)
ENDIF()

SRCS(socket_pair.cpp)

END()
