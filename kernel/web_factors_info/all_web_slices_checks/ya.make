RECURSE_FOR_TESTS(unallowed_factors_check)

RECURSE_FOR_TESTS(slices_check)

LIBRARY()

OWNER(
    ilnurkh
    g:factordev
)

PEERDIR(
    kernel/factors_info/all_slices_infos
)

END()
