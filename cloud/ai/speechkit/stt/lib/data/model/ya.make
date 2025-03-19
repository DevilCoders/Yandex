PY3_LIBRARY()

OWNER(
    o-gulyaev
)

PY_SRCS(
    common/__init__.py
    common/hash.py
    common/id.py
    common/s3_consts.py
    dao/__init__.py
    dao/common.py
    dao/common_markup.py
    dao/eval.py
    dao/markups_assignments.py
    dao/markups_params.py
    dao/markups_pools.py
    dao/metrics.py
    dao/recognitions.py
    dao/records.py
    dao/records_bits.py
    dao/records_bits_markups.py
    dao/records_joins.py
    dao/records_tags.py
    registry/__init__.py
    registry/tags.py
    tags/__init__.py
    tags/expressions.py
    tags/model.py
    tags/view.py
    __init__.py
)

PEERDIR(
    dataforge
    cloud/ai/lib/python/datetime
    cloud/ai/lib/python/serialization
    contrib/python/crcmod
)

END()
