OWNER(g:cpp-contrib)

UNITTEST()

DATA(
    arcadia_tests_data/catfilter
)

PEERDIR(
    ADDINCL kernel/catfilter
)

SRCDIR(kernel/catfilter)

SRCS(
    catfilter_ut.cpp
)

END()
