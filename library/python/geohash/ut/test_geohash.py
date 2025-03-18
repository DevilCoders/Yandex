from library.python.geohash import encode, encode_to_bits, decode, decode_to_bbox, decode_bits, cover_circle
import pytest


@pytest.fixture
def coords():
    return 55.75, 37.616667


@pytest.fixture
def geohash_str():
    return 'ucftpuzx7c5z'


@pytest.fixture
def geohash_bits():
    return 949654441010900159L


def test_encode(coords):
    return [encode(coords[0], coords[1], i) for i in xrange(4, 10)]


def test_encode_bits(coords):
    return [encode_to_bits(coords[0], coords[1], i) for i in xrange(4, 10)]


def test_decode(geohash_str):
    return [decode(geohash_str[:i]) for i in xrange(4, 10)]


def test_decode_to_bbox(geohash_str):
    return [decode_to_bbox(geohash_str[:i]) for i in xrange(4, 10)]


def test_decode_to_bbox_contistency(geohash_str):
    for i in xrange(4, 10):
        lat, lon = decode(geohash_str[:i])
        (lat_min, lon_min), (lat_max, lon_max) = decode_to_bbox(geohash_str[:i])
        assert (lat_max+lat_min)/2 == pytest.approx(lat)
        assert (lon_max+lon_min)/2 == pytest.approx(lon)


def test_decode_bits(geohash_bits):
    return [decode_bits(geohash_bits >> 5*i, i) for i in xrange(4, 10)]


def test_cover_circle(coords):
    return cover_circle(coords[0], coords[1], 150, 8)


def test_error():
    with pytest.raises(RuntimeError):
        encode(-180, -180)
