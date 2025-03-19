OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PY_SRCS(
    NAMESPACE dbaas_internal_api.modules.hadoop
    __init__.py
    charts.py
    compute_quota.py
    console.py
    constants.py
    create.py
    delete.py
    info.py
    jobs.py
    metadata.py
    modify.py
    operations.py
    pillar.py
    schemas.py
    start.py
    stop.py
    topology.py
    traits.py
    types.py
    ui_links.py
    utils.py
    validation.py
    version.py
)

PEERDIR(
    contrib/python/semver
)

END()
