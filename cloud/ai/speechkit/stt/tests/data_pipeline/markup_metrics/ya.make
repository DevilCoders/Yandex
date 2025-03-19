PY3TEST()

OWNER(g:dataforge o-gulyaev)

TEST_SRCS(
    test_markup_metrics.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/model
    cloud/ai/speechkit/stt/lib/data_pipeline/markup_metrics
)

SIZE(SMALL)

END()
