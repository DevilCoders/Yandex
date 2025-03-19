OWNER(
    rufrozen
    g:ugc
)

PY23_LIBRARY()

PY_SRCS(
    __init__.py
    feedback.pyx
)

PEERDIR(
    kernel/ugc/aggregation
    kernel/ugc/aggregation/proto
)

END()
