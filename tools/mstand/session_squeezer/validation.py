# coding=utf-8
import logging

import yaqutils.time_helpers as utime

from mstand_enums.mstand_online_enums import ServiceEnum


class SqueezeValidationException(Exception):
    @staticmethod
    def format_message(yuid, errors):
        errors = "\n".join(errors)
        return "SQUEEZE_VALIDATION_ERROR\n{}:\n{}".format(yuid, errors)


def validate_actions(yuid, day, service, fall_on_error, actions):
    """
    :type yuid: str
    :type day: datetime.date | None
    :type service: str
    :type fall_on_error: bool
    :type actions: list[dict[str]]
    """
    errors = list(_validate_actions(yuid, day, service, actions))
    if errors:
        if fall_on_error:
            message = SqueezeValidationException.format_message(yuid, errors)
            raise SqueezeValidationException(message)
        else:
            return False
    return True


def _validate_actions(yuid, day, service, actions):
    if day:
        dates = utime.DateRange(day, day)
        min_timestamp = dates.start_timestamp - utime.Period.HOUR
        max_timestamp = dates.end_timestamp + utime.Period.HOUR
    else:
        min_timestamp = 946684800  # 2000-01-01
        max_timestamp = 4102444800  # 2100-01-01
    requests = set()
    clicks = set()
    day_str = utime.format_date(day, pretty=True)

    for action in actions:
        field_errors = list(_validate_fields(action, min_timestamp, max_timestamp))
        if field_errors:
            for error in field_errors:
                yield error
            continue

        if service in ServiceEnum.SKIP_REQUEST_CHECK:
            continue

        action_type = action.get("type")
        timestamp = action["ts"]

        action_context = "{act} at {ts}, day={day}, yuid={yuid}".format(act=action_type,
                                                                        ts=timestamp,
                                                                        day=day_str,
                                                                        yuid=yuid)
        if action_type == "request":
            reqid = action.get("reqid")
            if not reqid:
                logging.warning("%s has no reqid", action_context)
            elif reqid in requests:
                yield "{} duplicates reqid {}".format(action_context, reqid)
            else:
                requests.add(reqid)
        elif action_type in ["click", "dynamic-click"]:
            reqid = action.get("reqid")
            if not reqid:
                logging.warning("%s has no reqid", action_context)
            else:
                clicks.add(reqid)
                if reqid not in requests:
                    logging.warning("%s before request: %s", action_context, reqid)
    clicks_without_requests = clicks - requests
    if clicks_without_requests:
        action_context = "day={}, yuid={}".format(day_str, yuid)
        yield "ReqIDs of clicks without requests for {}: {}".format(action_context, sorted(clicks_without_requests))


def _validate_fields(action, min_timestamp, max_timestamp):
    action_type = action.get("type", "unknown action")

    timestamp = action.get("ts")
    if not timestamp:
        yield "{} has no timestamp".format(action_type)
    elif timestamp < min_timestamp:
        yield "{} {} has too low timestamp".format(action_type, timestamp)
    elif timestamp > max_timestamp:
        yield "{} {} has too big timestamp".format(action_type, timestamp)

    action_index = action.get("action_index")
    if action_index is None:
        yield "{} {} has no action_index".format(action_type, timestamp)
