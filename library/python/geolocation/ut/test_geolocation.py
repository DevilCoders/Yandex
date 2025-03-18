import library.python.geolocation as geolocation
import pytest


@pytest.fixture
def lyon():
    return 45.7597, 4.8422


@pytest.fixture
def paris():
    return 48.8567, 2.3508


@pytest.fixture
def lyon_paris_distance_km():
    return 392


def test_calc_distance(lyon, paris, lyon_paris_distance_km):
    distance = geolocation.calc_distance(lyon[0], lyon[1], paris[0], paris[1])
    assert round(distance) == lyon_paris_distance_km, distance
