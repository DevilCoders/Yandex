OWNER(g:antiadblock)

PY2TEST()

INCLUDE(${ARCADIA_ROOT}/antiadblock/postgres_local/recipe/postgresql_bin.inc)

DATA(
    arcadia/antiadblock/configs_api/migrations
    arcadia/antiadblock/configs_api/tests/functional/tickets
)

TEST_SRCS(
    conftest.py
    test_argus_api.py
    test_audit.py
    test_auth.py
    test_configs.py
    test_configs_service_api.py
    test_idm.py
    test_internal_api.py
    test_metrics_api.py
    test_search.py
    test_smoke.py
    test_sonar_api.py
    test_utils_api.py
    test_validation.py
    test_dashboard.py
    test_startrek.py
)

PEERDIR(
    contrib/python/PyHamcrest
    contrib/python/requests
    contrib/python/pytest
    contrib/python/mock
    contrib/python/wsgi-intercept
    contrib/python/PyJWT

    antiadblock/postgres_local
    antiadblock/configs_api/lib
)

REQUIREMENTS(network:full)

SIZE(MEDIUM)

END()
