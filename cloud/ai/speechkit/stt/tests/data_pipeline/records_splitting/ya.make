PY3TEST()

OWNER(o-gulyaev)

TEST_SRCS(
    test_get_bits_indices.py
)

PEERDIR(
    contrib/python/ujson
    cloud/ai/speechkit/stt/lib/data_pipeline/records_splitting
)

SIZE(SMALL)

END()