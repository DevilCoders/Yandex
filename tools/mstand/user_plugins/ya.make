OWNER(
    g:mstand
)

PY3_LIBRARY()

PEERDIR(
    tools/mstand/mstand_utils
    quality/yaqlib/yaqutils
)

PY_SRCS(
    NAMESPACE user_plugins
    __init__.py
    plugin_container.py
    plugin_tests.py
    plugin_helpers.py
    plugin_key.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
