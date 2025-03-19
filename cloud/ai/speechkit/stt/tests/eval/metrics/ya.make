PY3TEST()

OWNER(
    o-gulyaev
)

TEST_SRCS(
    test_calculation.py
    test_mer.py
    test_wer.py
    test_wer_ex.py
    test_levenshtein.py
)

PEERDIR(
    contrib/python/ujson
    cloud/ai/speechkit/stt/lib/eval
)



END()
