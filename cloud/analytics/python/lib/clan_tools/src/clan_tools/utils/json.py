import typing as tp
import numpy as np
import simplejson as json  # type: ignore
import datetime
from time import mktime

from clan_tools.utils.time import to_utc


def json_serialize(obj: tp.Any, datetime_to_utc: int) -> tp.Any:
    if isinstance(obj, datetime.datetime):
        return _parse_dt(obj, datetime_to_utc)
    elif isinstance(obj, np.int64):
        return int(obj)
    elif isinstance(obj, np.ndarray):
        return list(obj)
    elif isinstance(obj, tuple):
        return str(obj)
    else:
        try:
            return obj.__dict__
        except:
            return str(obj)


def _parse_dt(dt: datetime.datetime, datetime_to_utc: int) -> tp.Union[str, int]:
    if datetime_to_utc == 1:
        return int(mktime(dt.timetuple()) * 1000)
    elif datetime_to_utc == 2:
        return to_utc(dt)
    else:
        return str(dt)


def json_dumps(obj: tp.Dict[str, tp.Any], datetime_to_utc: bool = True, indent: int = 4) -> str:
    bytes_json = json.dumps(
        obj,
        default=lambda obj: json_serialize(obj, datetime_to_utc),
        separators=(',', ':'),
        indent=indent
    )
    return bytes_json.encode('utf-8')

__all__ = ['json_serialize', 'json_dumps']
