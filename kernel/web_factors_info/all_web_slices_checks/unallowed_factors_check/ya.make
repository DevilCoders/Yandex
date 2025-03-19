UNITTEST("unallowed_factors_check")

OWNER(
    ilnurkh
    ulanovgeorgiy
    g:factordev
)

SIZE(MEDIUM)

PEERDIR(
    kernel/web_factors_info/all_web_slices_checks
    kernel/web_factors_info/validators
    search/formula_chooser/archive_checker
)

SRCS(
    unallowed_factors_check_ut.cpp
)

END()
