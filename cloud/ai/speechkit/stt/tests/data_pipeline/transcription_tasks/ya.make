PY3TEST()

OWNER(o-gulyaev)

TEST_SRCS(
    test_get_bit_index.py
    test_get_bit_overlap.py
    test_get_bits_overlaps.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data_pipeline/transcription_tasks
)

SIZE(SMALL)

END()
