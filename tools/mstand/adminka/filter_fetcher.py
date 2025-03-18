import logging

from typing import Optional

import experiment_pool

from adminka import ab_observation
from adminka.ab_cache import AdminkaCachedApi
from experiment_pool import Pool  # noqa

service_request_filter = {
    "portal", "web", "images", "video", "cbir", "maps",
    "weather", "market", "news", "turbo", "collections",
    "mail"
}


def libra_supports_filter(name: str,
                          value: Optional[str] = None) -> bool:
    # see: trunk/arcadia/quality/user_sessions/request_aggregate_lib/filters.cpp
    if name.startswith("portal_filter_"):
        return True

    if name == "service_request_filter":
        if value is None:
            return True

        return set(value.split()) <= service_request_filter

    if name in [
        "geo_original_filter",
        "user_region_filter",
        "user_outer_region_filter",
        "experiment_filter",
        "query_filter",
        "relev_filter",
        "relev_string_filter",
        "search_props_filter",
        "reg_search_props_filter",
        "full_request_reg_filter",
        "full_request_meta_filter",
        "rearr_filter",
        "words_per_query_filter",
        "query_language_filter",
        "ui_language_filter",
        "domain_filter",
        "uitype_filter",
        "cgi_filter",
        "wizard_filter",
        "referer_filter",
        "special_placement_filter",
        "blockstat_filter",
        "blockstat_count_filter",
        "blockvars_filter",
        "blockstats_filter",
        "miscclick_filter",
        "useragent_filter",
        "misspell_filter",
        "misspell_fields_filter",
        "misspell_fields_filter_v2",
        "navig_answer_filter",
        "browser_filter",
        "instant_search_filter",
        "query_reg_filter",
        "reqid_reg_filter",
        "markers_filter",
        "page_nums_filter",
        "page_not_adv_filter",
        "reask_filter",
        "serp_page_loaded_filter",
        "uid_filter",
        "has_fuid_filter",
        "diff_tdi_filter",
        "antirobot_threat_filter",
        "triggered_testids_filter",
        "geo_properties_filter",
        "events_path_filter",
        "events_vars_filter",
        "result_height_filter",
        "video_type_filter",
        "market_report_cgi_filter",
        "producing_reqid_reg_filter",
        "yandex_login_filter",
        "visible_filter",
        "x_region_filter",
    ]:
        return True

    return False


def fetch_all(pool: Pool,
              session: Optional[AdminkaCachedApi] = None,
              allow_bad_filters: bool = False,
              ignore_triggered_testids_filter: bool = False) -> None:
    if session is None:
        session = AdminkaCachedApi()

    logging.info("Fetching filters for pool")
    for obs in pool.observations:
        logging.debug("--> observation %s", obs)
        fetch_obs(obs, session, allow_bad_filters, ignore_triggered_testids_filter)


def fetch_obs(obs: experiment_pool.Observation,
              session: AdminkaCachedApi,
              allow_bad_filters: bool = False,
              ignore_triggered_testids_filter: bool = False) -> None:
    if obs.id:
        obs_info = session.get_observation_info(obs.id)
        obs.filters = ab_observation.parse_filters(obs_info, ignore_triggered_testids_filter)

        for name, value in obs.filters.libra_filters:
            if not libra_supports_filter(name, value) and not allow_bad_filters:
                raise Exception("libra doesn't support filter {} (observation {})".format(name, obs.id))

        logging.debug("   --> loaded filters: %s", obs.filters)
    else:
        logging.debug("   --> no id, skipped")
