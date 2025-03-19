PY3_PROGRAM()

OWNER(
    o-gulyaev
)

PY_SRCS(
    MAIN get_cluster_references.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/text/cluster_references
    library/python/nirvana
)

END()
