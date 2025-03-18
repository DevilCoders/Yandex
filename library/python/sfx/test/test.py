import pytest
from yatest.common import binary_path, execute


@pytest.fixture
def sfx():
    return binary_path('library/python/sfx/bin/sfx')


def test_unpack(sfx, tmpdir):
    execute([sfx, sfx, '-o', str(tmpdir)])


def test_symlink(sfx, tmpdir):
    src = str(tmpdir.mkdir('src'))
    sym = str(tmpdir.mkdir('sym'))
    execute([sfx, sfx, '-o', src])
    execute([sfx, sfx, '-o', sym, '-s', src])
