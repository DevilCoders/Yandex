PY3_LIBRARY()

OWNER(pg)

PEERDIR(
    contrib/python/PyYAML
    library/python/resource
)

TEST_SRCS(
    conftest.py
    test_cpp.py
)

RESOURCE(
    devtools/ya/handlers/style/style_config /cpp_style/config
)

END()
