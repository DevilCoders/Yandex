LIBRARY()

OWNER(
    ilnurkh
    insight
    g:neural-search
)

SRCS(
    optimized_multiply_add.cpp
)


IF (ARCH_X86_64)
    SRC_C_AVX2(
        optimized_multiply_add_avx2.cpp
    )
    SRC_C_SSE41(
        optimized_multiply_add_sse.cpp
    )
ELSE()
    SRCS(
        optimized_multiply_add_stub.cpp
    )
ENDIF()

END()

RECURSE_FOR_TESTS(
    tests
    bench
)
