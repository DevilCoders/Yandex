PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    contrib/python/humanfriendly
    contrib/python/marshmallow-dataclass
    contrib/python/pyaml
)

PY_SRCS(
    __init__.py
    models/clickhouse.py
    models/kafka.py
    models/__init__.py
    models/base.py
    models/resource.py
    product.py
    disk_size.py
    data_sources/__init__.py
    data_sources/base.py
    data_sources/disk.py
    data_sources/geo.py
    data_sources/flavor.py
    schema.py
)

END()
