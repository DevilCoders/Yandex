OWNER(eeight)

LIBRARY()

IF (ARCH_X86_64 OR ARCH_I386)
    IF (OS_LINUX OR OS_DARWIN OR CLANG_CL)
        SRC(select_in_word_bmi.cpp -mbmi2 -mbmi)
        CFLAGS(-DUSE_BMI)
    ELSE()
        IF (MSVC)
            SRC(select_in_word_bmi.cpp /D__BMI2__=1 /D__BMI__=1)
            CFLAGS(/DUSE_BMI)
        ENDIF()
    ENDIF()
ENDIF()

SRCS(
    select_in_word.cpp
    select_in_word_x86.cpp
)

PEERDIR(
    library/cpp/pop_count
)

END()
