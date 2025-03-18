from library.python.filesys import temp_dir, touch
import os.path


def test_touch():
    touch('123')
    assert os.path.exists('123')


def test_temp_dir():
    with temp_dir() as d:
        filename = os.path.join(d, '123')
        touch(filename)
        assert os.path.exists(filename)
    assert not os.path.exists(filename)
