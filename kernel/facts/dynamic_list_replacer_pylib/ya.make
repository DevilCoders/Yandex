PY23_LIBRARY()

OWNER(
    g:facts
)

PEERDIR(
    kernel/facts/dynamic_list_replacer
)

PY_SRCS(
    __init__.py
    wrapper.pyx
)

END()

RECURSE_FOR_TESTS(
    test
)
