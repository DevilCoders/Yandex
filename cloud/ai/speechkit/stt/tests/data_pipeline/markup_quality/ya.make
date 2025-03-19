PY3TEST()

OWNER(o-gulyaev)

TEST_SRCS(
    test_acceptance.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/model
    cloud/ai/speechkit/stt/lib/data_pipeline/markup_quality
)

SIZE(SMALL)

END()
