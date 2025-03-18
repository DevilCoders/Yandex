from mstand_enums.mstand_online_enums import FiltersEnum


def filter_user(actions):
    return_all = any(action.data["is_match"] for action in actions)
    if return_all:
        return actions
    else:
        return []


def filter_from_first_day(actions):
    from_date = None
    for action in actions:
        if action.data["is_match"]:
            from_date = action.date
            break
    result_actions = []
    if from_date is not None:
        for action in actions:
            if action.date >= from_date:
                result_actions.append(action)
    return result_actions


def filter_from_first_ts(actions):
    from_time = None
    for action in actions:
        if action.data["is_match"]:
            from_time = action.data["ts"]
            break
    result_actions = []
    if from_time is not None:
        for action in actions:
            if action.data["ts"] >= from_time:
                result_actions.append(action)
    return result_actions


def filter_day(actions):
    days = set()
    for action in actions:
        if action.data["is_match"]:
            days.add(action.date)
    result_actions = []
    for action in actions:
        if action.date in days:
            result_actions.append(action)
    return result_actions


FILTERS_DEFINITIONS = {
    FiltersEnum.NONE: None,
    FiltersEnum.USER: filter_user,
    FiltersEnum.DAY: filter_day,
    FiltersEnum.FROM_FIRST_DAY: filter_from_first_day,
    FiltersEnum.FROM_FIRST_TS: filter_from_first_ts,
}


def check_filter_correctness(name):
    if name is not None and name not in FILTERS_DEFINITIONS:
        raise Exception("Filter '{}' doesn't exist".format(name))


def get_filter(name):
    check_filter_correctness(name)
    if name is None:
        return None
    else:
        return FILTERS_DEFINITIONS[name]
