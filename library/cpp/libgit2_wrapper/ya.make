LIBRARY()

OWNER(g:arc)

IF (NOT MSVC)
    CXXFLAGS(-Wimplicit-fallthrough)
ENDIF()

SRCS(
    apply.cpp
    auth.cpp
    callbacks.cpp
    diff.cpp
    index.cpp
    GLOBAL init.cpp
    object_db.cpp
    object_id.cpp
    reference.cpp
    remote.cpp
    repository.cpp
    revwalk.cpp
    signature.cpp
    tree.cpp
    unidiff.cpp
)

PEERDIR(
    contrib/libs/libgit2
    library/cpp/colorizer
    library/cpp/openssl/init
)

END()

RECURSE_FOR_TESTS(ut)
