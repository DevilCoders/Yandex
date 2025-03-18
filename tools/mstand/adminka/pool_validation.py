import collections
import datetime
import logging
import time

import adminka.ab_settings as ab_settings
import adminka.pool_enrich_services as admserv
import mstand_utils.testid_helpers as utestid
import yaqutils.requests_helpers as urequests

from adminka.ab_cache import AdminkaCachedApi  # noqa
from adminka.ab_observation import parse_observation, ObservationParseException
from mstand_enums.mstand_online_enums import ServiceEnum


def check_extra_testids(observation, session):
    if observation.id is None:
        logging.debug("--> skipping observation with no ID")
        return None

    if not str(observation.id).isdigit():
        logging.debug("--> skipping observation with non-numeric ID")
        return None

    try:
        obs_info = session.get_observation_info(observation.id)
    except urequests.RequestPageNotFoundError:
        return "AB observation does not exist"

    try:
        obs_parsed = parse_observation(obs_info, session, allow_adv=True)
    except ObservationParseException as ex:
        return "AB observation is incorrect: {}".format(ex.message)

    our_testids = set(observation.all_testids())
    adm_testids = set(obs_parsed.all_testids())
    extra_testids = our_testids - adm_testids
    missing_testids = adm_testids - our_testids

    logging.debug("  --> testids: in pool: %s, in ab: %s, extra: %s, missing: %s",
                  our_testids, adm_testids, extra_testids, missing_testids)

    if missing_testids:
        logging.warning("  --> missing testids: %s", missing_testids)

    if extra_testids:
        return "Extra testids not in AB: {}".format(extra_testids)

    if observation.control.testid != obs_parsed.control.testid:
        return "Control testid not equal in AB: {} != {}".format(observation.control.testid, obs_parsed.control.testid)

    return None


def check_restriction(restrictions, key, value, experiment):
    if not value:
        logging.debug("      --> skipping restriction check with no value")
        return None

    whitelist = set()
    blacklist = set()
    for item in restrictions:
        if item:
            if item.startswith("-"):
                blacklist.add(item[1:])
            else:
                whitelist.add(item)

    logging.debug(
        "      --> checking restriction %s: expecting: %s, blacklist: %s, whitelist: %s",
        key, value, blacklist, whitelist
    )

    if isinstance(value, (tuple, list)):
        in_black = any(v in blacklist for v in value)
        in_white = any(v in whitelist for v in value)
    else:
        in_black = value in blacklist
        in_white = value in whitelist

    if blacklist and in_black:
        return "TestID {} has wrong restriction {}: expected {}, AB blacklist: {}".format(experiment.testid,
                                                                                          key,
                                                                                          value,
                                                                                          blacklist)

    if whitelist and not in_white:
        return "TestID {} has wrong restriction {}: expected {}, AB whitelist: {}".format(experiment.testid,
                                                                                          key,
                                                                                          value,
                                                                                          whitelist)

    return None


SERVICE_TO_ABT_PLATFORM = {
    ServiceEnum.WEB_DESKTOP: ("desktop",),
    ServiceEnum.WEB_DESKTOP_EXTENDED: ("desktop",),
    ServiceEnum.WEB_TOUCH: ("touch", "tablet"),
    ServiceEnum.WEB_TOUCH_EXTENDED: ("touch", "tablet"),
}

SERVICE_TO_ABT_SERVICE = {
    ServiceEnum.WEB_DESKTOP: ("web",),
    ServiceEnum.WEB_DESKTOP_EXTENDED: ("web",),
    ServiceEnum.WEB_TOUCH: ("web", "touch", "padsearch"),
    ServiceEnum.WEB_TOUCH_EXTENDED: ("web", "touch", "padsearch"),
}


def check_restriction_for_service(footprints, service, experiment):
    logging.debug("    --> checking restrictions on service %s", service)
    errors = []
    for footprint in footprints:
        restrictions = footprint.get("restrictions", {})
        platform_restrictions = set(restrictions["platforms"].split(","))
        service_restrictions = set(restrictions["services"].split(","))

        platform_err = check_restriction(platform_restrictions,
                                         "platforms",  # for logging purposes
                                         SERVICE_TO_ABT_PLATFORM.get(service),
                                         experiment)

        service_err = check_restriction(service_restrictions,
                                        "services",
                                        SERVICE_TO_ABT_SERVICE.get(service),
                                        experiment)

        if not platform_err and not service_err:
            return []

        if platform_err:
            errors.append(platform_err)

        if service_err:
            errors.append(service_err)

    return errors


def check_experiment_dates(observation, experiment, session, cli_services):
    if experiment.testid is None:
        logging.info("  --> skipping experiment with no testid")
        return None
    if utestid.testid_is_all(experiment.testid):
        logging.info("  --> skipping period checking for '%s' testid value", experiment.testid)
        return None
    if not utestid.testid_is_simple(experiment.testid):
        logging.info("  --> skipping period checking for unknown '%s' testid value", experiment.testid)
        return None

    ab_as_mstand_services = admserv.get_mstand_services(observation, experiment, session)

    error_template = "TestID {} was not enabled on these dates {} for '{}'."

    if not ab_as_mstand_services:
        return error_template.format(experiment.testid, observation.dates, "all")

    errors = []
    for mstand_service in ServiceEnum.convert_aliases(cli_services):
        if mstand_service in ServiceEnum.AUTO:
            for service in ServiceEnum.extend_services([mstand_service]):
                if service in ab_as_mstand_services:
                    break
            else:
                errors.append(
                    error_template.format(experiment.testid, observation.dates, mstand_service)
                )
        elif mstand_service in ab_settings.MSTAND_TO_ABT_RULES and mstand_service not in ab_as_mstand_services:
            errors.append(
                error_template.format(experiment.testid, observation.dates, mstand_service)
            )

    if errors:
        return "\n".join(errors)


class PoolValidationErrors(object):
    def __init__(self):
        self.errors = collections.defaultdict(list)
        self.error_testids = collections.defaultdict(list)

    def add_error(self, observation, result, testid=None):
        if result is not None:
            self.errors[observation].append(result)
        if testid is not None:
            self.error_testids[observation].append(testid)

    def pretty_print(self, show_ok=False):
        result = []

        for observation, errors in self.errors.items():
            if errors:
                result.append("Observation {}".format(observation))
                for error in errors:
                    result.append("- {}".format(error))
            elif show_ok:
                result.append("Observation {}: OK".format(observation))

        return "\n".join(result)

    def all_error_testids(self):
        return self.error_testids

    def is_ok(self):
        return not self.errors

    def crash_on_error(self):
        if not self.is_ok():
            raise ValueError("Pool validation failed:\n{}".format(self.pretty_print()))


def validate_pool(pool, session, services=None):
    logging.info("validating pool")

    error_storage = PoolValidationErrors()
    time_start = time.time()

    for observation in pool.observations:
        logging.info("--> validating observation %s", observation)

        obs_error_message = check_extra_testids(observation, session)
        if obs_error_message is not None:
            error_storage.add_error(observation, obs_error_message)
        if services is None:
            logging.warning("Observation %r services was not setup", observation.id)
            continue

        for experiment in observation.all_experiments():
            logging.info("  --> checking experiment %s", experiment)
            exp_error_message = check_experiment_dates(observation, experiment, session, services)
            if exp_error_message is not None:
                error_storage.add_error(observation, exp_error_message, experiment.testid)

    time_end = time.time()

    logging.info(
        "Pool (observations: %d, experiments: %d) validated in %s",
        len(pool.observations),
        sum(len(observation.experiments) + 1 for observation in pool.observations),
        datetime.timedelta(seconds=time_end - time_start),
    )

    return error_storage


def init_observation_services(observation, session, cli_services, use_filters, error_storage=None,
                              allow_fake_services=False):
    """
    :type observation: experiment_pool.observation.Observation
    :type session: AdminkaCachedApi
    :type cli_services: list[str]
    :type use_filters: bool
    :type error_storage: PoolValidationErrors | None
    :type allow_fake_services: bool
    :rtype: PoolValidationErrors()
    """
    if error_storage is None:
        error_storage = PoolValidationErrors()

    try:
        observation.services = admserv.extend_services_from_observation(observation, session, cli_services,
                                                                        use_filters, allow_fake_services)
    except Exception as exc:
        error_storage.add_error(observation, exc.message)

    if not observation.services:
        error_storage.add_error(observation,
                                "Observation {} does not contain a list of services".format(observation.id))
    else:
        logging.info("Observation %s target services: %r", observation.id, observation.services)

    return error_storage


def init_pool_services(pool, session, cli_services, use_filters, allow_fake_services=False):
    """
    :type pool: experiment_pool.pool.Pool
    :type session: AdminkaCachedApi
    :type cli_services: list[str]
    :type use_filters: bool
    :type allow_fake_services: bool
    :rtype: PoolValidationErrors()
    """
    error_storage = PoolValidationErrors()

    for observation in pool.observations:
        init_observation_services(observation, session, cli_services, use_filters,
                                  error_storage, allow_fake_services)

    return error_storage
