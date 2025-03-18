OWNER(g:cpp-contrib)

LIBRARY()

SRCS(
    lex.rl5
    funs.cpp
    out.cpp
    def.h
    face.h
    lex.h
    result.h
)

PEERDIR(
    library/cpp/html/spec
)

# todo: check if these flags are properly cleared
SET(
    RL_FLAGS
    -C
    -e
)

SET(RLCG_FLAGS -G2)

END()
