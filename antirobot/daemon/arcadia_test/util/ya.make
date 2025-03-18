PY3_LIBRARY()

OWNER(
    g:antirobot
)

PY_SRCS(
    __init__.py
    asserts.py
    daemon.py
    captcha_page.py
    req_sender.py
    spravka.py
    mock.py
    captcha_cloud_api_mock.py
    captcha_mock.py
    cbb_mock.py
    discovery_mock.py
    fury_mock.py
    resource_manager_mock.py
    access_service_mock.py
    unified_agent_mock.py
    wizard_mock.py
    ydb_mock.py
    AntirobotTestSuite.py
)

PEERDIR(
    antirobot/captcha_cloud_api/proto
    antirobot/idl
    antirobot/scripts/antirobot_cacher_daemonlog
    antirobot/scripts/antirobot_eventlog
    antirobot/scripts/antirobot_processor_daemonlog
    contrib/libs/grpc/src/python/grpcio_status
    contrib/python/dateutil
    contrib/python/pytest
    library/python/cityhash
    library/python/testing/yatest_common
    ydb/public/sdk/python
)

END()
