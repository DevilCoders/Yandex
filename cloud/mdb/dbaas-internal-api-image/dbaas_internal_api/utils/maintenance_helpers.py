from copy import deepcopy
from datetime import datetime, timedelta

MAINTENANCE_TO_ISO_WEEKDAYS = {
    'MON': 1,
    'TUE': 2,
    'WED': 3,
    'THU': 4,
    'FRI': 5,
    'SAT': 6,
    'SUN': 7,
}


def calculate_nearest_maintenance_window(operation_time: datetime, window_day: str, window_hour: int) -> datetime:
    window_iso_weekday = MAINTENANCE_TO_ISO_WEEKDAYS[window_day]
    operation_iso_weekday = operation_time.isoweekday()
    next_available_window = deepcopy(operation_time)
    if operation_iso_weekday == window_iso_weekday:
        if operation_time.hour >= window_hour - 1:
            next_available_window += timedelta(weeks=1)
    elif operation_iso_weekday < window_iso_weekday:
        next_available_window += timedelta(days=window_iso_weekday - operation_iso_weekday)
    else:
        next_available_window += timedelta(days=7 + window_iso_weekday - operation_iso_weekday)

    next_available_window = next_available_window.replace(hour=window_hour - 1)

    return next_available_window
