"""Various data formatting tools"""

import datetime
import re
import humanize
import operator
import time

from yc_common import constants


_UNDERSCORE_RE = re.compile(r"_([a-z])")
_CAMELCASE_RE = re.compile(r"[A-Z]")
_HYPHEN_RE = re.compile(r"-([a-z])")


class KeepFormat:
    def __init__(self, obj):
        self.obj = obj


def human_size(size):
    units = (
        ("PiB", constants.PETABYTE),
        ("TiB", constants.TERABYTE),
        ("GiB", constants.GIGABYTE),
        ("MiB", constants.MEGABYTE),
        ("KiB", constants.KILOBYTE),
    )

    for unit_name, unit_size in units:
        if size >= unit_size and size % unit_size == 0:
            break
    else:
        unit_name, unit_size = "Bytes", 1

    return "{} {}".format(size // unit_size, unit_name)


def parse_human_size(string, allow_zero=True):
    try:
        try:
            value = int(string)
        except ValueError:
            value = int(string[:-1])

            try:
                value *= {
                    "K": constants.KILOBYTE,
                    "M": constants.MEGABYTE,
                    "G": constants.GIGABYTE,
                    "T": constants.TERABYTE,
                    "P": constants.PETABYTE,
                }[string[-1:]]
            except KeyError:
                raise ValueError

        if value < 0 or value == 0 and not allow_zero:
            raise ValueError
    except ValueError:
        raise ValueError("Invalid size")

    return value


def human_size_fractional(size):
    return humanize.naturalsize(size, binary=True)


def human_duration(seconds):
    return humanize.naturaldelta(seconds)


def human_timeout(timeout):
    return humanize.naturaltime(timeout, future=True)


def human_relative_time(other_time):
    seconds = other_time - time.time()
    return humanize.naturaltime(abs(seconds), future=seconds > 0)


def parse_human_duration(string, allow_zero=True):
    string = string.strip()

    try:
        value = int(string[:-1])

        try:
            value *= {
                "s": 1,
                "m": constants.MINUTE_SECONDS,
                "h": constants.HOUR_SECONDS,
                "d": constants.DAY_SECONDS,
            }[string[-1:]]
        except KeyError:
            raise ValueError

        if value < 0 or value == 0 and not allow_zero:
            raise ValueError
    except ValueError:
        raise ValueError("Invalid duration")

    return value


def parse_human_time(time_string):
    """Parses time specified by user in various formats like time, date or time period."""

    if "." in time_string or ":" in time_string:
        return _parse_time(time_string)
    else:
        cur_time = int(time.time())

        try:
            period = parse_human_duration(time_string, allow_zero=False)
            if period > cur_time:
                raise ValueError
        except ValueError:
            raise ValueError("Invalid time specification: {!r}".format(time_string))

        return cur_time - period


# Intended to be used in help messages
parse_human_time.human_format = (
    "'$number[smhd]|$time|$date|$time $date$|$date $time' "
    "where $number[smhd] is for example 3d for 3 day period, "
    "$time is %H:%M|%H:%M:%S and $date is %d.%m.%Y|%Y.%m.%d")


def _parse_time(time_string):
    orig_time_string = time_string

    # Reformat the string to "$time $date" format if it's "$date $time"
    time_string = " ".join(sorted(
        filter(bool, map(operator.methodcaller("strip"), time_string.split(" "))),
        key=lambda s: "." in s))

    dot_count = time_string.count(".")
    colon_count = time_string.count(":")

    time_format = ""
    has_date = False

    try:
        if colon_count:
            if colon_count == 1:
                time_format = "%H:%M"
            elif colon_count == 2:
                time_format = "%H:%M:%S"
            else:
                raise ValueError

        if dot_count:
            has_date = True
            if time_format:
                time_format += " "

            if dot_count != 2:
                raise ValueError

            if len(time_string.split(" ")[-1].split(".")[0]) == 4:
                time_format += "%Y.%m.%d"
            else:
                time_format += "%d.%m.%Y"

        if not time_format:
            raise ValueError

        time_datetime = datetime.datetime.strptime(time_string, time_format)
        if not has_date:
            time_datetime = datetime.datetime.combine(datetime.date.today(), time_datetime.time())

        return int(time.mktime(time_datetime.timetuple()))
    except ValueError:
        raise ValueError("Invalid time specification: {!r}".format(orig_time_string))


def format_time(time):
    return datetime.datetime.fromtimestamp(time).strftime("%Y.%m.%d %H:%M:%S")


def underscore_to_lowercamelcase(obj):
    """Convert underscore style to lowercamelcase style"""

    def _converter(obj):
        return _UNDERSCORE_RE.sub(lambda m: m.group(1).upper(), obj)

    return _convert_key(obj, _converter)


def camelcase_to_underscore(obj):
    """Convert camelcase style to underscore style"""

    def _converter(obj):
        return _CAMELCASE_RE.sub(lambda m: "_" + m.group().lower(), obj)

    return _convert_key(obj, _converter)


def hyphen_to_lowercamelcase(obj):
    """Convert hyphen style to lowercamelcase style example-str -> exampleStr"""

    def _converter(obj):
        return _HYPHEN_RE.sub(lambda m: m.group(1).upper(), obj)

    return _convert_key(obj, _converter)

def hyphen_to_underscore(obj):
    """Convert hyphen style to uppercamelcase style example-str -> EXAMPLE_STR"""

    def _converter(obj):
        return obj.upper().replace("-", "_")

    return _convert_key(obj, _converter)


def _convert_key(obj, converter):
    """Helper method to convert dictionaries"""

    if isinstance(obj, KeepFormat):
        return obj.obj
    if isinstance(obj, dict):
        return {_convert_key(k, converter): _convert_value(v, converter) for k, v in obj.items()}
    elif isinstance(obj, str):
        return converter(obj)
    else:
        return obj


def _convert_value(obj, converter):
    """Helper method to convert values of dictionary objects"""

    if isinstance(obj, KeepFormat):
        return obj.obj
    elif isinstance(obj, dict):
        return _convert_key(obj, converter)
    elif isinstance(obj, list):
        return [_convert_value(v, converter) for v in obj]
    else:
        return obj
