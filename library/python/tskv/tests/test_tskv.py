# coding: utf-8
import library.python.tskv as lptskv
import pytest


pairs_loads = (
    ("a=1\tb=2\tc=3", {b'a': b'1', b'b': b'2', b'c': b'3'}),
    ("b=2", {b'b': b'2'}),
    ("c=3\n", {b'c': b'3\n'}),
    ("фыв=ячс", {b'\xd1\x84\xd1\x8b\xd0\xb2': b'\xd1\x8f\xd1\x87\xd1\x81'}),
    (u"фыв=ячс", {b'\xd1\x84\xd1\x8b\xd0\xb2': b'\xd1\x8f\xd1\x87\xd1\x81'}),
)

pairs_dumps = (
    (b"a=1\tb=2\tc=3", {b'a': b'1', b'b': b'2', b'c': b'3'}),
    (b"b=2", {b'b': b'2'}),
    (b"c=3\\n", {b'c': b'3\n'}),
    (b"\xd1\x84\xd1\x8b\xd0\xb2=\xd1\x8f\xd1\x87\xd1\x81", {b'\xd1\x84\xd1\x8b\xd0\xb2': b'\xd1\x8f\xd1\x87\xd1\x81'}),
)


@pytest.mark.parametrize('s,d', pairs_loads)
def test_loads(s, d):
    assert lptskv.loads(s) == d


@pytest.mark.parametrize('s,d', pairs_dumps)
def test_dumps(s, d):
    assert lptskv.dumps(d) == s


def test_empty_load():
    with pytest.raises(ValueError):
        lptskv.loads('')


def test_broken_tskv_load():
    with pytest.raises(ValueError):
        lptskv.loads('a=1\t')


def test_unicode_keys_dump():
    with pytest.raises((UnicodeEncodeError, TypeError)):
        lptskv.dumps({u"фыва": u"йцук"})
