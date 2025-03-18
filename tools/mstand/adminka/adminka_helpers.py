import logging

import yaqutils.requests_helpers as urequests


# noinspection PyClassHasNoInit
class AbtErrorMsg:
    PERM_ERROR = "ABT_PERM_ERROR"
    TEMP_ERROR = "ABT_TEMP_ERROR"


def fetch_observation_ticket(session, obs):
    """
    :type session:
    :type obs: Observation
    :rtype: str
    """
    if obs.id is None:
        return None
    try:
        obs_info = session.get_observation_info(obs.id)
        return obs_info["ticket"]
    except urequests.RequestPageNotFoundError as exc:
        logging.warning("Could not fetch ticket info for observation %s: %s", obs, exc)
        return AbtErrorMsg.PERM_ERROR
    except Exception as exc:
        logging.warning("Could not fetch ticket info for observation %s after some attempts: %s", obs, exc)
        return AbtErrorMsg.TEMP_ERROR


def fetch_abt_experiment_field(session, exp, field_name):
    """
    :type session:
    :type exp: Experiment
    :type field_name: str
    :rtype: str
    """
    if exp.testid is None:
        return None
    try:
        testid_info = session.get_testid_info(exp.testid)
        return testid_info.get(field_name)
    except urequests.RequestPageNotFoundError as exc:
        logging.error("Could not fetch field '%s' for experiment %s: %s", field_name, exp, exc)
        return AbtErrorMsg.PERM_ERROR
    except Exception as exc:
        logging.error("Could not fetch field '%s' for experiment %s after some attempts: %s", field_name, exp, exc)
        return AbtErrorMsg.TEMP_ERROR
