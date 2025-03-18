OWNER(
    g:mstand
)

PY3_LIBRARY()

PEERDIR(
    quality/yaqlib/yaqutils
    quality/yaqlib/yaqab
    tools/mstand/mstand_enums
    tools/mstand/mstand_utils
    tools/mstand/mstand_structs
    tools/mstand/user_plugins
)

PY_SRCS(
    NAMESPACE adminka
    __init__.py
    ab_cache.py
    ab_helpers.py
    ab_observation.py
    ab_settings.py
    ab_task.py
    activity.py
    adminka_helpers.py
    control_pool.py
    date_validation.py
    fetch_observations.py
    filter_fetcher.py
    filter_pool.py
    pool_enrich_services.py
    pool_fetcher.py
    pool_validation.py
    testid.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
