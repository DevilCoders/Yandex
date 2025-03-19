UNITTEST()

OWNER(
    g:jupiter
)

PEERDIR(
    library/cpp/archive
    library/cpp/packers
    kernel/sent_lens/utlib
    ADDINCL kernel/sent_lens
)

ARCHIVE(
    NAME test_data.inc
    sent_lens_writer_test_input.inc
    sent_lens_writer_test_output.inc
)

SRCDIR(
    kernel/sent_lens
)

SRCS(
    sent_lens_ut.cpp
    sent_lens_writer_ut.cpp
)

END()
