LIBRARY()

OWNER(epar)

PEERDIR(
    kernel/extended_mx_calcer/calcers
    kernel/extended_mx_calcer/interface
    kernel/extended_mx_calcer/factory
    kernel/extended_mx_calcer/proto
)

END()

RECURSE_FOR_TESTS(tools/calcers_test)
