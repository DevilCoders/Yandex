import adminka.ab_cache
import adminka.filter_fetcher
import adminka.pool_validation


def validate_and_enrich(pool, session, add_filters=True, services=None, ignore_triggered_testids_filter=False,
                        allow_fake_services=False, allow_bad_filters=False):
    """
    :type pool: experiment_pool.pool.Pool()
    :type session: adminka.ab_cache.AdminkaCachedApi()
    :type add_filters: bool
    :type services: list[str]
    :type ignore_triggered_testids_filter: bool
    :type allow_fake_services: bool
    :type allow_bad_filters: bool
    """
    adminka.pool_validation.validate_pool(pool, session, services).crash_on_error()
    if services:
        adminka.pool_validation.init_pool_services(pool, session, services, add_filters,
                                                   allow_fake_services).crash_on_error()
    if add_filters:
        adminka.filter_fetcher.fetch_all(
            pool=pool,
            session=session,
            allow_bad_filters=allow_bad_filters,
            ignore_triggered_testids_filter=ignore_triggered_testids_filter,
        )
