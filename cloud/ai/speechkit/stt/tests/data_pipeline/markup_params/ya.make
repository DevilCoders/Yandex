PY3TEST()

OWNER(o-gulyaev)

TEST_SRCS(
    test_markup_params.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data_pipeline/markup_params
    cloud/ai/speechkit/stt/lib/data_pipeline/transcript_feedback_loop
    contrib/python/mock
)

SIZE(SMALL)

REQUIREMENTS(ram:12)

END()
