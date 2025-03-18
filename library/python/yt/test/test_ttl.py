from datetime import timedelta

import dateutil.parser
import pytest

from library.python.yt.ttl import expiration_time


def test_expiration_time():
    expiration_time(minutes=1)
    expiration_time(days=1, hours=2)
    with pytest.raises(TypeError):
        expiration_time(ttl=timedelta(minues=1), days=1, hours=2)

    now = dateutil.parser.parse("2017-06-18T21:14:20.219383Z")

    assert expiration_time(now=now, minutes=1) == "2017-06-18T21:15:20.219383+00:00"
    assert expiration_time(now=now, days=1, hours=2) == "2017-06-19T23:14:20.219383+00:00"

    assert expiration_time(now=now, ttl=timedelta(minutes=1)) == "2017-06-18T21:15:20.219383+00:00"
    assert expiration_time(now=now, ttl=timedelta(days=1, hours=2)) == "2017-06-19T23:14:20.219383+00:00"
