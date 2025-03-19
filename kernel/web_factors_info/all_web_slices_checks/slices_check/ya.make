# see SUBBOTNIK-1602 and IGNIETFERRO-1818
IF (NOT OS_WINDOWS)
    UNITTEST("slices_check")

    OWNER(
        ilnurkh
        g:factordev
    )

    PEERDIR(
        kernel/web_factors_info/all_web_slices_checks
        kernel/web_factors_info/validators
    )

    SRCDIR(kernel/web_factors_info)

    SRCS(
        slices_check_ut.cpp
    )

    END()
ENDIF()
