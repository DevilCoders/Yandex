from datetime import datetime, timedelta

import dateutil.tz


def expiration_time(**kws):
    """Shorthand function for calculating `expiration_time` attribute value.

    >>> expiration_time = expiration_time(hours=24)
    >>> ttl = timedelta(hours=1, minutes=30)
    >>> expiration_time = expiration_time(ttl=ttl)
    """

    now = kws.pop('now', None)
    if now is None:
        now = datetime.now(dateutil.tz.tzutc())

    ttl = kws.pop('ttl', None)
    if ttl is None:
        ttl = timedelta(**kws)
    elif kws:
        raise TypeError("No extra arguments are allowed when `ttl` is specified")

    expiration_time = now + ttl
    return expiration_time.isoformat()


def set_ttl(yt, path, **kws):
    """Set `expiration_time` attribute for `path` based on ttl value.

    >>> set_ttl(yt, "//tmp/example", hours=1)
    """
    return yt.set_attribute(path, 'expiration_time', expiration_time(**kws))
