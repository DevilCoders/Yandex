OWNER(g:cpp-contrib)

Y_BENCHMARK()

PEERDIR(
    contrib/libs/taocrypt
    library/cpp/digest/sfh
    library/cpp/digest/crc32c
    library/cpp/digest/old_crc
    library/cpp/digest/murmur
    library/cpp/digest/md5
)

IF (MSVC)
    CFLAGS(-DFARMHASH_NO_BUILTIN_EXPECT=1)
ELSE()
    IF (ARCH_X86_64)
        CFLAGS(
            -DFARMHASH_ASSUME_SSE42
            -msse4.2
            -DFARMHASH_ASSUME_AESNI
            -maes
        )
    ENDIF()
ENDIF()

IF (OS_LINUX)
    PEERDIR(
        yabs/server/util
    )
    CFLAGS(-DY_BOBHASH=1)
ENDIF()

SRCS(
    SpookyV2.cpp
    main.cpp
    farmhash.cc
)

END()
