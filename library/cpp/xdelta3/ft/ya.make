GTEST()

OWNER(g:bsyeti)

SIZE(LARGE)

TAG(ya:fat)

IF (SANITIZER_TYPE != "memory")
    SRCS(
        merge_patches_ut.cpp
    )

    PEERDIR(
        library/cpp/xdelta3/state
        library/cpp/xdelta3/ut/rand_data
        
        contrib/libs/xdelta3
    )
ENDIF()

END()
