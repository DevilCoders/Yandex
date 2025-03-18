LIBRARY()

OWNER(g:arc)

CXXFLAGS(-Wimplicit-fallthrough)

SRCS(mrw_lock.cpp)

END()

RECURSE_FOR_TESTS(ut)
