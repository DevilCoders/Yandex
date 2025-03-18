OWNER(
    g:mstand
)

PY3_LIBRARY()

PEERDIR(
    tools/mstand/mstand_utils
)

PY_SRCS(
    NAMESPACE mstand_structs
    __init__.py
    lamp_key.py
    squeeze_versions.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
