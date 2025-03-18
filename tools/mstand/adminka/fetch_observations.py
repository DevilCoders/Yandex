import logging

import adminka.ab_observation as adm_obs
from experiment_pool import Observation  # noqa


def get_observations(
        session,
        obs_ids=None,
        dates=None,
        tag=None,
        extra_data=False,
        validate_testids=True,
):
    """
    :type session: CachedApi
    :type obs_ids: list[str]
    :type dates: utime.DateRange
    :type tag:
    :type extra_data: bool
    :type validate_testids: bool
    :rtype: __generator[Observation]
    """
    logging.info("getting observations, obs_ids: %s, %s, tag %s", obs_ids, dates, tag)

    allow_adv = False
    observations_list = []
    if obs_ids:
        logging.info("getting observations by %d ids", len(obs_ids))
        observations_list = [session.get_observation_info(observation_id) for observation_id in obs_ids]
        allow_adv = True
    elif dates or tag:
        logging.info("getting observations by date range %s, tagged %s", dates, tag)
        observations_list = session.get_observation_list(dates, tag=tag)

    logging.info("got %s observations", len(observations_list))
    logging.info("parsing fetched observations")

    for raw_observation in observations_list:
        try:
            current_id = adm_obs.parse_observation_id(raw_observation)
            current_dates = adm_obs.parse_observaiton_dates(raw_observation, current_id)

            if not current_dates.start:
                logging.warning("discarding observation %s: no start date", current_id)
                continue

            if dates and dates.has_date() and not current_dates.intersect(dates):
                logging.warning("discarding observation %s: dates don't intersect %s", current_id, current_dates)
                continue

            if dates and dates.start and current_dates.start < dates.start:
                logging.warning("discarding observation %s: start date has to be greater or equal than date_from", current_id)
                continue

            observation = adm_obs.parse_observation(
                raw_observation,
                session,
                allow_adv=allow_adv,
                validate_testids=validate_testids,
            )

            if not obs_ids and not observation.experiments:
                # ignore observations with one testid (but keep them for obs_id mode)
                logging.warning("discarding observation %s: not enough testids", observation)
                continue

            if extra_data:
                observation.extra_data = get_extra_data_for_observation(session, raw_observation)

            yield observation
        except adm_obs.ObservationParseException as ex:
            logging.warning("discarding observation ID %s: %s", ex.obs_id, ex.message)

    logging.info("getting observations done")


def get_observation_by_id(session, obs_id, extra_data=False, validate_testids=True):
    """
    :type session: CachedApi
    :type obs_id: int
    :type extra_data: bool
    :type validate_testids: bool
    :rtype: Observation
    """

    logging.info("getting observation by id: %s", obs_id)
    try:
        raw_observation = session.get_observation_info(obs_id)
    except Exception as e:
        logging.info("cannot get observation %s: %s", obs_id, e)
        raise

    logging.info("parsing fetched observation")

    observation = adm_obs.parse_observation(
        raw_observation,
        session,
        validate_testids=validate_testids
    )
    if not observation.dates.start:
        logging.warning("discarding observation %s: no start date", observation)

    if extra_data:
        observation.extra_data = get_extra_data_for_observation(session, raw_observation)

    return observation


def get_extra_data_for_observation(session, raw_observation):
    testids_info = [
        get_testid_info(session, testid)
        for testid in raw_observation["testids"]
    ]
    extra_data = {
        "adminka": raw_observation,
        "testids_info": testids_info,
    }
    ticket = raw_observation.get("ticket")
    try:
        raw_task = session.get_task_info_for_ticket(ticket)
    except Exception as e:
        logging.info("cannot get task %s: %s", ticket, e)
        raw_task = None
    if raw_task is not None:
        extra_data["adminka_task"] = raw_task
    return extra_data


def get_testid_info(session, testid):
    testid_info = session.get_testid_info(testid)
    return {
        "testid": testid,
        "name": testid_info.get("title", ""),
    }
