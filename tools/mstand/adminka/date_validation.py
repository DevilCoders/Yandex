import datetime
import logging

import adminka.activity
import mstand_utils.testid_helpers as utestid
import yaqutils.time_helpers as utime
from experiment_pool import Pool


def get_all_enabled_ranges(session, testids):
    logging.debug("get_all_enabled_ranges testids: %s", testids)
    all_events = []
    for testid in testids:
        for pair in adminka.activity.enabled_event_pairs(session.get_testid_activity(testid)):
            all_events.append(pair.on)
            if pair.off:
                all_events.append(pair.off)

    all_events = sorted(all_events, key=adminka.activity.date_from_event)
    logging.debug("get_all_enabled_ranges all_events: %s", all_events)

    exp_states = {str(testid): False for testid in testids}
    all_enabled = False
    all_enabled_date = None
    all_enabled_ranges = []

    evt_date = None

    for event in all_events:
        evt_date = adminka.activity.date_from_event(event)
        evt_testid = str(event["testid"])
        if event["status"] == "on":
            exp_states[evt_testid] = True
        elif event["status"] == "off":
            exp_states[evt_testid] = False

        all_enabled_new = all(exp_states.values())
        if all_enabled_new and not all_enabled:
            all_enabled_date = evt_date

        if not all_enabled_new and all_enabled:
            if all_enabled_date <= evt_date:
                all_enabled_ranges.append(utime.DateRange(all_enabled_date, evt_date))
                all_enabled_date = None

        all_enabled = all_enabled_new

    if evt_date is not None and all(exp_states.values()):
        all_enabled_ranges.append(utime.DateRange(evt_date, None))

    return all_enabled_ranges


def min_date(a, b):
    if a is None:
        return b
    if b is None:
        return a
    return min(a, b)


def max_date(a, b):
    if a is None:
        return b
    if b is None:
        return a
    return max(a, b)


def _check_dates(dates, obs_dates, include_start_date, include_end_date, req_dates):
    if not include_start_date:
        dates.start += datetime.timedelta(days=1)
    if not include_end_date and dates.end is not None:
        dates.end -= datetime.timedelta(days=1)

    dates = dates.intersect(obs_dates)
    if dates is None:
        logging.debug("-- Enabled dates are outside observation dates, skipping")
        return None

    if req_dates is None:
        return dates

    dates = dates.intersect(req_dates)
    if dates:
        logging.debug("-- Observation was enabled on requested dates; final dates: %s", dates)
        return dates


def check_observation_dates(observation, session, include_start_date, include_end_date, req_dates):
    """
    :type observation: Observation
    :type session: adminka.cache.CachedApi
    :type include_start_date: bool
    :type include_end_date: bool
    :type req_dates:
    :return:
    """
    logging.debug("Checking enabled dates for observation %s", observation)
    testids = observation.all_testids()

    if all(not utestid.testid_from_adminka(t) for t in testids):
        logging.debug("Got observation with non-ab-supported experiments")
        return _check_dates(observation.dates, observation.dates, include_start_date, include_end_date, req_dates)

    assert all(utestid.testid_from_adminka(t) for t in testids), "All testids have to be ab-supported"

    for on_off_dates in get_all_enabled_ranges(session, testids):
        logging.debug("-- Found enabled dates: %s", on_off_dates)
        final_dates = _check_dates(on_off_dates, observation.dates, include_start_date, include_end_date, req_dates)

        if final_dates is not None:
            return final_dates

    logging.debug("-- Observation was not enabled on requested dates, discarding")
    return None


def fix_pool_dates(session, pool, include_start_date, include_end_date, req_dates, strict=False):
    """
    :type session: AdminkaCachedApi
    :type pool: Pool
    :type include_start_date: bool
    :type include_end_date: bool
    :type req_dates: utime.DateRange
    :type strict: bool
    :return: Pool
    """
    logging.info("validating observation dates")

    new_pool = Pool()
    for observation in pool.observations:
        dates = check_observation_dates(observation, session, include_start_date, include_end_date, req_dates)
        if dates:
            observation.dates = dates
            new_pool.observations.append(observation)
        elif strict:
            raise Exception("Observation {} was not enabled on requested dates".format(observation))
    logging.info("observation dates validation done")
    return new_pool
