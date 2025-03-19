PY3TEST()

OWNER(o-gulyaev)

TEST_SRCS(
    test_select_records_joins.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data_pipeline/select_records_joins
)

SIZE(SMALL)

END()
