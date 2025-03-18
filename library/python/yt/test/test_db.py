import tempfile

import library.python.yt as lpy


class FakeClient(object):
    def __init__(self):
        self._db = {}

    def set(self, k, v):
        self._db[k] = v

    def get(self, k):
        return self._db.get(k)

    def create(self, *args, **kwargs):
        pass

    def Transaction(self):
        return tempfile.TemporaryFile()

    def lock(self, *args, **kwargs):
        pass


def test_db_1():
    def encode(v):
        return [[v]]

    def decode(v):
        return v[0][0]

    db = lpy.KiparisDB(FakeClient(), '', encode=encode, decode=decode)

    def func(v):
        if not v:
            return [1]

        return v + [v[-1] + 1]

    assert db.repr('x') is None
    assert db.apply(func, 'x') == [1]
    assert db.apply(func, 'x') == [1, 2]
    assert db.apply(func, 'x') == [1, 2, 3]
