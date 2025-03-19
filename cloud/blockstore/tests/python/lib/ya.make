PY3_LIBRARY()

OWNER(g:cloud-nbs)

PEERDIR(
    cloud/blockstore/config

    cloud/blockstore/public/sdk/python/client
    cloud/blockstore/public/sdk/python/protos

    cloud/storage/core/tools/common/python

    kikimr/ci/libraries

    contrib/python/requests
    contrib/python/retrying
)

PY_SRCS(
    __init__.py
    access_service.py
    disk_agent_runner.py
    endpoints.py
    loadtest_env.py
    nbs_http_proxy.py
    nbs_runner.py
    nonreplicated_setup.py
    stats.py
    test_base.py
    test_with_plugin.py
)

END()
