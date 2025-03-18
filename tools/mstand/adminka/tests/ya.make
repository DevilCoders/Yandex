PY3TEST()

OWNER(
    g:mstand
)

TEST_SRCS(
    tools/mstand/adminka/ab_observation_ut.py
    tools/mstand/adminka/activity_ut.py
    tools/mstand/adminka/adminka_helpers_ut.py
    tools/mstand/adminka/conftest.py
    tools/mstand/adminka/control_pool_ut.py
    tools/mstand/adminka/filter_pool_ut.py
    tools/mstand/adminka/pool_enrich_services_ut.py
    tools/mstand/adminka/pool_fetcher_ut.py
    tools/mstand/adminka/pool_validation_ut.py
)

PEERDIR(
    tools/mstand/adminka
    tools/mstand/experiment_pool
)

DATA(
    arcadia/tools/mstand/adminka/tests/data/collections/cache.json
    arcadia/tools/mstand/adminka/tests/data/collections/pool.json
    arcadia/tools/mstand/adminka/tests/data/market_sessions_stat/cache.json
    arcadia/tools/mstand/adminka/tests/data/market_sessions_stat/pool.json
    arcadia/tools/mstand/adminka/tests/data/mobile_app/cache.json
    arcadia/tools/mstand/adminka/tests/data/mobile_app/pool.json
    arcadia/tools/mstand/adminka/tests/data/toloka/cache.json
    arcadia/tools/mstand/adminka/tests/data/toloka/pool.json
    arcadia/tools/mstand/adminka/tests/data/web_auto/cache.json
    arcadia/tools/mstand/adminka/tests/data/web_auto/pool.json
    arcadia/tools/mstand/adminka/tests/data/web_auto_and_morda/cache.json
    arcadia/tools/mstand/adminka/tests/data/web_auto_and_morda/pool.json
    arcadia/tools/mstand/adminka/tests/data/cache.json
)

SIZE(SMALL)

END()
