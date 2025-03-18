UNITTEST()

OWNER(g:arc)

CXXFLAGS(-Wimplicit-fallthrough)

SRCS(mrw_lock_ut.cpp)

PEERDIR(library/cpp/fuse/mrw_lock)

END()
