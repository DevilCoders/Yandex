PY3TEST()

OWNER(o-gulyaev)

TEST_SRCS(
    data.py
    test_join.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data_pipeline/join
    cloud/ai/speechkit/stt/lib/data_pipeline/markup_params
    contrib/python/mock
)

SIZE(SMALL)

END()
