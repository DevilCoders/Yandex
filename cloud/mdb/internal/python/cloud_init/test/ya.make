PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    cloud/mdb/internal/python/cloud_init
)

TEST_SRCS(
    test_yaml_modules.py
    test_yaml.py
)

END()
