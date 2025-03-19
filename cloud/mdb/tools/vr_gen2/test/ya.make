PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

SIZE(MEDIUM)

FORK_SUBTESTS()

REQUIREMENTS(
    cpu:4
    ram:12
)

PEERDIR(
    cloud/mdb/tools/vr_gen2/internal
    contrib/python/PyHamcrest
)

TEST_SRCS(
    test_resources.py
    test_product.py
    test_models.py
    test_disk_sizes.py
)

RESOURCE_FILES(
    resources/empty/defs.yaml
    resources/unknown_property/defs.yaml
    resources/clickhouse/defs.yaml
    resources/kafka/defs.yaml
    resources/datasource/disk_type.yaml
    resources/datasource/flavor.yaml
    resources/datasource/geo.yaml
)

END()
