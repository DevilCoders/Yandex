import library.python.base64 as abase64
import base64


def test_compat():
    for s in (b'', b'1', b'a', b'ab', b'abcd', b'abcde'):
        assert base64.b64encode(s) == abase64.b64encode(s)
        assert abase64.b64decode(abase64.b64encode(s)) == s


def test_big():
    for s in (b'1', b'a', b'ab', b'abcd', b'abcde'):
        s = s * 128

        assert base64.b64encode(s) == abase64.b64encode(s)
        assert abase64.b64decode(abase64.b64encode(s)) == s
