PY3TEST()

OWNER(
    o-gulyaev
)

TEST_SRCS(
    test_records_joins.py
    test_serialization.py
    test_yt.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/ops
)



END()
