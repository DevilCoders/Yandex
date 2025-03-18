OWNER(
    ulyanov
    velavokr
    g:antimalware
    g:antiwebspam
)

UNITTEST()

DATA(arcadia_tests_data/infected_masks)

PEERDIR(
    library/cpp/infected_masks
)

SRCS(
    infected_masks_ut.cpp
    sb_masks_ut.cpp
    masks_comptrie_ut.cpp
)

END()
