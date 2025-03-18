import datetime
import logging

import adminka.ab_observation
import yaqutils.time_helpers as utime

from adminka.ab_cache import AdminkaCachedApi
from experiment_pool import Pool, Experiment, Observation


def split_dates_by_max(dates, max_range=14):
    """
    :type dates: yaqutils.time_helpers.DateRange
    :type max_range: int
    :rtype: __generator[utils.DateRange]
    """
    assert max_range > 0
    start = dates.start
    while start <= dates.end:
        end = min(dates.end, start + datetime.timedelta(days=max_range - 1))
        yield utime.DateRange(start, end)
        start = end + datetime.timedelta(days=1)


def generate_salt_change_ranges(date_range, salt_change_dates, salt_change_dates_manual, use_split_change_day):
    """
    :type date_range: utils.time_helpers.DateRange
    :type salt_change_dates: set[datetime.date]
    :type salt_change_dates_manual: set[datetime.date]
    :type use_split_change_day: bool
    :rtype: __generator[utils.DateRange]
    """
    assert date_range.start
    assert all(scd in date_range for scd in salt_change_dates)

    start = date_range.start
    sorted_dates = sorted(salt_change_dates)
    logging.debug("    --> salt change dates: %s", sorted_dates)

    for change in sorted_dates:
        end = change - datetime.timedelta(days=1)
        if start <= end:
            yield utime.DateRange(start, end)
        if use_split_change_day and change not in salt_change_dates_manual:
            start = change
        else:
            start = change + datetime.timedelta(days=1)

    end = date_range.end
    if end and start <= end:
        yield utime.DateRange(start, end)


def get_salt_change_dates(testids, dates, session):
    """
    :type testids: list[str]
    :type dates: yaqutils.time_helpers.DateRange
    :type session: AdminkaCachedApi
    :rtype: (set[datetime.date], set[datetime.date])
    """
    salt_changes = session.get_split_change_info(testids, dates)
    salt_change_dates = set()
    salt_change_dates_manual = set()
    for date, change in salt_changes.items():
        parsed_date = utime.parse_date_msk(date)
        salt_change_dates.add(parsed_date)
        if any(testid["manual"] for testid in change.values()):
            logging.warning("    --> detected manual salt change on %s", date)
            salt_change_dates_manual.add(parsed_date)
    return salt_change_dates, salt_change_dates_manual


def generate_ranges_for_control_pool(
        dates,
        max_range,
        salt_change_dates,
        salt_change_dates_manual,
        use_split_change_day,
        force_single_days,
):
    """
    :type dates: yaqutils.time_helpers.DateRange
    :type max_range: int
    :type salt_change_dates: set[datetime.date]
    :type salt_change_dates_manual: set[datetime.date]
    :type use_split_change_day: bool
    :type force_single_days: bool
    :rtype: list[utils.time_helpers.DateRange]
    """
    logging.info("    --> generating same salt ranges")
    result = set()
    for dr in generate_salt_change_ranges(dates, salt_change_dates, salt_change_dates_manual, use_split_change_day):
        if dr.number_of_days() <= max_range:
            logging.debug("      --> same salt range: %s (%s days)", dr, dr.number_of_days())
            result.add(dr)
        else:
            logging.debug("      --> will split too long range: %s (%s days)", dr, dr.number_of_days())
            for dr_sub in split_dates_by_max(dr, max_range):
                logging.debug("      --> same salt range: %s (%s days)", dr_sub, dr_sub.number_of_days())
                result.add(dr_sub)

    if force_single_days:
        single = generate_salt_change_single(dates, salt_change_dates, salt_change_dates_manual, use_split_change_day)
        result.update(single)

    return sorted(result)


def generate_salt_change_single(dates, salt_change_dates, salt_change_dates_manual, use_split_change_day):
    """
    :type dates: yaqutils.time_helpers.DateRange
    :type salt_change_dates: set[datetime.date]
    :type salt_change_dates_manual: set[datetime.date]
    :type use_split_change_day: bool
    :rtype: __generator[utils.DateRange]
    """
    logging.info("    --> generating 1 day ranges")
    for date in dates:
        if use_split_change_day:
            if date in salt_change_dates_manual:
                logging.debug("      --> skipping salt change day: %s", utime.format_date(date))
                continue
        elif date in salt_change_dates:
            logging.debug("      --> skipping salt change day: %s", utime.format_date(date))
            continue
        dr = utime.DateRange(date, date)
        logging.debug("      --> day: %s", dr)
        yield dr


def get_ranges_for_control_pool(testids, dates, session, max_range, use_split_change_day, force_single_days):
    """
    :type testids: list[str]
    :type dates: yaqutils.time_helpers.DateRange
    :type session: AdminkaCachedApi
    :type max_range: int
    :type use_split_change_day: bool
    :type force_single_days: bool
    :rtype: list[utils.time_helpers.DateRange]
    """
    logging.info("  --> generating date ranges for testids %s on dates %s", testids, dates)

    salt_change_dates, salt_change_dates_manual = get_salt_change_dates(testids, dates, session)
    return generate_ranges_for_control_pool(
        dates=dates,
        max_range=max_range,
        salt_change_dates=salt_change_dates,
        salt_change_dates_manual=salt_change_dates_manual,
        use_split_change_day=use_split_change_day,
        force_single_days=force_single_days,
    )


def generate_control_pool(observations, dates, max_range, use_split_change_day, force_single_days):
    """
    :type observations: list[str]
    :type dates: yaqutils.time_helpers.DateRange
    :type max_range: int
    :type use_split_change_day: bool
    :type force_single_days: bool
    :rtype: Pool
    """
    session = AdminkaCachedApi()
    pool = Pool()

    logging.info("Loading observations from ABT...")

    parsed_obs = []
    for observation in observations:
        logging.info("--> Loading observation ID %s", observation)
        obs_info = session.get_observation_info(observation)
        parsed_obs.append(adminka.ab_observation.parse_observation(obs_info, session))

    logging.info("Generating new observations...")
    for obs in parsed_obs:
        logging.info("--> generating observations for observation ID %s", obs.id)
        ranges = get_ranges_for_control_pool(
            testids=obs.all_testids(),
            dates=dates,
            session=session,
            max_range=max_range,
            use_split_change_day=use_split_change_day,
            force_single_days=force_single_days,
        )
        for new_dates in ranges:
            new_control = Experiment(testid=obs.control.testid)
            new_experiments = [Experiment(testid=exp.testid) for exp in obs.all_experiments(include_control=False)]
            new_obs = Observation(
                obs_id=obs.id,
                dates=new_dates,
                sbs_ticket=obs.sbs_ticket,
                sbs_workflow_id=obs.sbs_workflow_id,
                sbs_metric_results=obs.sbs_metric_results,
                control=new_control,
                experiments=new_experiments,
            )
            pool.observations.append(new_obs)

    return pool
