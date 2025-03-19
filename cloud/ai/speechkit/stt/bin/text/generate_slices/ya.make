PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN generate_slices.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/text/slice/generation
    library/python/nirvana
)

END()
