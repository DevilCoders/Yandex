import logging

import mstand_utils.testid_helpers as utestid
import yaqutils.time_helpers as utime
from experiment_pool import Experiment, Observation, ObservationFilters


def dates_ok(observation, info):
    """
    :type observation: experiment_pool.pool.Observation
    :type info: dict[str]
    :rtype: bool
    """
    date_start = utime.parse_date_msk(info["datestart"])
    if not date_start or date_start > observation.dates.start:
        return False
    date_end = utime.parse_date_msk(info["dateend"])
    if date_end and date_end < observation.dates.end:
        return False
    return True


def parse_filters(observation_data, ignore_triggered_testids_filter=False):
    filter_hash = observation_data["uid"]
    filters = []
    has_triggered_testids_filter = False
    for fd in observation_data["filters"]:
        has_triggered_testids_filter |= fd["name"] == "triggered_testids_filter"
        if ignore_triggered_testids_filter and fd["name"] == "triggered_testids_filter":
            logging.warning('Ignore libra filter "%s".', fd["name"])
        else:
            filters.append((fd["name"], fd["value"]))
    return ObservationFilters(
        filters=filters,
        filter_hash=filter_hash,
        has_triggered_testids_filter=has_triggered_testids_filter,
    )


class ObservationParseException(Exception):
    def __init__(self, message, obs_id):
        self.message = message
        self.obs_id = obs_id

    def __str__(self):
        return "Observation {}: {}".format(self.obs_id, self.message)


def parse_sbs_info(sbs_info):
    arr = sbs_info.split("s")
    sbs_ticket_number = int(arr[0].strip())
    sbs_ticket = "SIDEBYSIDE-{}".format(sbs_ticket_number)
    sbs_system_id = arr[1].strip()
    return sbs_ticket, sbs_system_id


class SbsParseResult(object):
    def __init__(self, sbs_ticket=None, system_id_map=None):
        """
        :type sbs_ticket: str | None
        :type system_id_map: dict[str -> str] | None
        """
        self.sbs_ticket = sbs_ticket
        if not system_id_map:
            system_id_map = {}
        self.system_id_map = system_id_map


def parse_task_info(task_info, session, testids, obs_id):
    if not task_info:
        return SbsParseResult()

    sbs_dict = task_info.get("sbs")
    if not sbs_dict:
        return SbsParseResult()

    sbs_ticket = None
    system_id_map = {}

    for testid in testids:
        sbs_info = sbs_dict.get(testid)
        if not sbs_info:
            continue
        cur_ticket, system_id = parse_sbs_info(sbs_info)
        if not sbs_ticket:
            sbs_ticket = cur_ticket
        else:
            if cur_ticket != sbs_ticket:
                logging.warning("Found different sbs tickets in sbs info of observation %s: %s and %s, "
                                "sbs data will not be saved in observation", obs_id, sbs_ticket, cur_ticket)
                return SbsParseResult()
        if not system_id:
            raise ObservationParseException("Sbs system id is empty", obs_id)
        system_id_map[testid] = system_id

    if not sbs_ticket:
        return SbsParseResult()

    if len(system_id_map) == 1:
        logging.warning("Only one sbs system found in sbs info of observation %s, ticket %s, "
                        "sbs data will not be saved in observation", obs_id, sbs_ticket)
        return SbsParseResult()

    return SbsParseResult(sbs_ticket, system_id_map)


def parse_observation(observation_data, session, allow_adv=True, validate_testids=True):
    obs_id = parse_observation_id(observation_data)

    logging.info("parsing observation %s data from AB", obs_id)
    testids = observation_data.get("testids", [])

    if not testids:
        raise ObservationParseException("Has no testids", obs_id)

    testids = [str(t) for t in testids]

    if validate_testids:
        if not allow_adv and any(utestid.testid_is_adv(t) for t in testids):
            raise ObservationParseException("Has adv testids", obs_id)

    dates = parse_observaiton_dates(observation_data, obs_id)

    # normalize data type (convert all testids to strings)
    str_testids = [str(testid) for testid in testids]
    control_testid = str_testids[0]
    experiment_testids = str_testids[1:]

    task_info = session.get_task_info_for_ticket(observation_data.get("ticket"))
    sbs_parse_result = parse_task_info(task_info, session, str_testids, obs_id)
    system_id_map = sbs_parse_result.system_id_map

    control = Experiment(testid=control_testid, sbs_system_id=system_id_map.get(control_testid))
    experiments = [Experiment(testid=testid, sbs_system_id=system_id_map.get(testid)) for testid in experiment_testids]

    obs_filters = parse_filters(observation_data)
    tags = observation_data.get("tags")

    return Observation(
        obs_id=obs_id,
        dates=dates,
        sbs_ticket=sbs_parse_result.sbs_ticket,
        control=control,
        experiments=experiments,
        filters=obs_filters,
        tags=tags,
    )


def parse_observation_id(observation_data):
    obs_id = str(observation_data["obs_id"]) if "obs_id" in observation_data else None
    return obs_id


def parse_observaiton_dates(observation_data, obs_id):
    date_from = utime.parse_date_msk(observation_data.get("datestart"))
    date_to = utime.parse_date_msk(observation_data.get("dateend"))
    try:
        dates = utime.DateRange(date_from, date_to)
    except utime.DateException:
        raise ObservationParseException("Start date: {} is > end date: {}".format(date_from, date_to), obs_id)
    return dates
