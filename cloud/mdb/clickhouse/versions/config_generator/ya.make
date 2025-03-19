OWNER(g:mdb)

PY3_PROGRAM(config-generator)

STYLE_PYTHON()

ALL_PY_SRCS(RECURSIVE)

PY_MAIN(cloud.mdb.clickhouse.versions.config_generator.main:main)

PEERDIR(
    cloud/mdb/clickhouse/versions/lib
    contrib/python/Jinja2
    contrib/python/more-itertools
    contrib/python/ruamel.yaml
)

END()
