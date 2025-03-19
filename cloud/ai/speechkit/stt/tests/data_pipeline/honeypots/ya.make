PY3TEST()

OWNER(o-gulyaev)

TEST_SRCS(
    test_generate_check_transcript_honeypots.py
    test_generate_transcript_honeypots.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data_pipeline/honeypots
    contrib/python/mock
)

SIZE(SMALL)

END()
