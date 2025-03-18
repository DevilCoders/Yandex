#!/usr/bin/env python2.7
# -*- coding: utf-8 -*-

import argparse
import itertools
import collections
import logging

import adminka.ab_settings as ab_settings
import adminka.activity as adm_activity
import adminka.date_validation as date_validation
import experiment_pool.pool_helpers as upool
import mstand_utils.args_helpers as mstand_uargs
import mstand_utils.testid_helpers as utestid
import yaqutils.misc_helpers as umisc
import yaqutils.time_helpers as utime

from adminka.ab_cache import AdminkaCachedApi
from experiment_pool import Experiment, Observation, Pool  # noqa
from mstand_enums.mstand_online_enums import ServiceEnum

ABT_PLATFORM_TO_WEB = {
    "desktop": ServiceEnum.WEB_DESKTOP,
    "touch": ServiceEnum.WEB_TOUCH,
    "tablet": ServiceEnum.WEB_TOUCH,
}

ABT_PLATFORM_TO_WEB_EXTENDED = {
    "desktop": ServiceEnum.WEB_DESKTOP_EXTENDED,
    "touch": ServiceEnum.WEB_TOUCH_EXTENDED,
    "tablet": ServiceEnum.WEB_TOUCH_EXTENDED,
}


def get_web_services_from_filters(observation_id, session):
    if observation_id is None:
        return False, set()

    obs_info = session.get_observation_info(observation_id)

    services = set()
    has_ui_filters = False
    for filter_node in obs_info["filters"]:
        if filter_node["name"] == "uitype_filter":
            has_ui_filters = True
            filters = filter_node["value"].split(" ")
            for filter_value in filters:
                if filter_value == "desktop":
                    services.add(ServiceEnum.WEB_DESKTOP)
                    services.add(ServiceEnum.WEB_DESKTOP_EXTENDED)
                elif filter_value in {"touch", "pad", "mobile", "mobileapp"}:
                    services.add(ServiceEnum.WEB_TOUCH)
                    services.add(ServiceEnum.WEB_TOUCH_EXTENDED)

    return has_ui_filters, services


def get_lists(restrictions, key):
    """
    :type restrictions: dict
    :type key: str
    :rtype tuple[set[str], set[str])
    """
    values_raw = restrictions.get(key, "").replace(" ", "")
    whitelist = set()
    blacklist = set()
    if values_raw:
        for item in values_raw.split(","):
            if item.startswith("-"):
                blacklist.add(item[1:])
            else:
                whitelist.add(item)

    assert not (blacklist and whitelist)

    return whitelist, blacklist


def get_platforms(restrictions):
    """
    :type restrictions: dict
    :rtype set[str]
    """
    whitelist, blacklist = get_lists(restrictions, "platforms")

    if whitelist:
        return whitelist
    return set(ab_settings.ABT_PLATFORMS).difference(blacklist)


def check_mstand_service(mstand_service, ab_whitelist, ab_blacklist, ab_platforms):
    """
    :type mstand_service: str
    :type ab_whitelist: set[str]
    :type ab_blacklist: set[str]
    :type ab_platforms: set[str]
    :rtype: bool
    """
    if mstand_service not in ab_settings.MSTAND_TO_ABT_RULES:
        return True

    rules = ab_settings.MSTAND_TO_ABT_RULES[mstand_service]
    for ab_platform, ab_service in itertools.product(*rules):
        if ab_whitelist:
            if ab_platform in ab_platforms and ab_service in ab_whitelist:
                return True
        else:
            if ab_platform in ab_platforms and ab_service not in ab_blacklist:
                return True

    return False


def get_possible_mstand_services(restrictions):
    """
    :type restrictions: dict
    :rtype: list[str]
    """
    ab_platforms = get_platforms(restrictions)
    ab_whitelist, ab_blacklist = get_lists(restrictions, "services")

    return [
        mstand_service
        for mstand_service in ServiceEnum.ALL
        if check_mstand_service(mstand_service, ab_whitelist, ab_blacklist, ab_platforms)
    ]


def parse_event(observation, on_event, off_event, dates):
    """
    :type observation: Observation
    :type on_event: dict
    :type off_event: dict | None
    :type dates: collections.defaultdict[str, set[date]]
    """
    start = date_validation.max_date(
        adm_activity.date_from_event(on_event),
        observation.dates.start
    )
    end = date_validation.min_date(
        adm_activity.date_from_event(off_event),
        observation.dates.end
    )
    if start > end:
        return

    available_days = list(utime.DateRange(start, end))
    for footprint in on_event.get("footprints", {}):
        restrictions = footprint.get("restrictions", {})
        for mstand_services in get_possible_mstand_services(restrictions):
            dates[mstand_services].update(available_days)


def get_mstand_services(observation, experiment, session):
    """
    :type observation: Observation
    :type experiment: Experiment
    :type session: AdminkaCachedApi
    :rtype: set[str]
    """
    if not utestid.testid_is_simple(experiment.testid):
        return ServiceEnum.ALL

    dates = collections.defaultdict(set)
    activity = session.get_testid_activity(experiment.testid)

    for pair in adm_activity.enabled_event_pairs(activity):
        for on, off in pair:
            parse_event(observation, on, off, dates)

    return {
        key
        for key, value in dates.items()
        if all(day in value for day in observation.dates)
    }


def extend_services_from_observation(observation, session, cli_services, use_filters=False,
                                     skip_services_checking=False):
    """
    :type observation: Observation
    :type session: AdminkaCachedApi
    :type cli_services: list[str]
    :type use_filters: bool
    :type skip_services_checking: bool
    :rtype: list[str]
    """
    logging.info("Extending services from observation %s...", observation)
    if use_filters:
        has_ui_filters, web_services_from_filters = get_web_services_from_filters(observation.id, session)
    else:
        has_ui_filters = False
        web_services_from_filters = None

    ab_as_mstand_services = get_mstand_services(observation, observation.control, session)
    for experiment in observation.experiments:
        ab_as_mstand_services &= get_mstand_services(observation, experiment, session)

    web_services = set()
    other_services = set()
    bad_services = set()
    for mstand_service in ServiceEnum.convert_aliases(cli_services):
        if mstand_service in ServiceEnum.AUTO:
            for service in ServiceEnum.extend_services([mstand_service]):
                if service in ab_as_mstand_services:
                    web_services.add(service)
        elif mstand_service in ServiceEnum.WEB_ALL:
            if mstand_service in ab_as_mstand_services:
                web_services.add(mstand_service)
            else:
                bad_services.add(mstand_service)
        elif mstand_service in ab_as_mstand_services or skip_services_checking:
            other_services.add(mstand_service)
        else:
            bad_services.add(mstand_service)

    if bad_services:
        raise Exception("Observation {} is not available for services {}.".format(observation.id, sorted(bad_services)))

    if has_ui_filters:
        web_services &= web_services_from_filters

    if web_services:
        logging.info("Observation %s is available for web services: %s",
                     observation, umisc.to_lines(web_services))

    if other_services:
        logging.info("Observation %s is available for non-web services: %s",
                     observation, umisc.to_lines(other_services))

    logging.info("Services were extended from observation %s", observation)
    return sorted(web_services | other_services)


def parse_args():
    parser = argparse.ArgumentParser(description="Validate services from pool by ABT")
    mstand_uargs.add_input_pool(parser)
    mstand_uargs.add_list_of_services(parser, default="web-auto")
    mstand_uargs.add_use_filters_flag(parser)
    return parser.parse_args()


def main():
    cli_args = parse_args()

    session = AdminkaCachedApi()
    pool = upool.load_pool(cli_args.input_file)

    for observation in pool.observations:
        print(observation.id, extend_services_from_observation(observation, session, cli_args.services, cli_args.use_filters))
        for experiment in observation.all_experiments():
            print(experiment.testid, get_mstand_services(observation, experiment, session))


if __name__ == "__main__":
    main()
