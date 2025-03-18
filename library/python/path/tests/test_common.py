import pytest

import library.python.path as lib


@pytest.mark.parametrize('args', ((), ('',), ('/',), ('///',)), ids=('none', 'empty', 'root', 'norm'))
def test_path_empty(args):
    path = lib.Path(*args)
    assert isinstance(path, tuple)
    assert isinstance(path, lib.Path)
    assert path == ()
    assert len(path) == 0
    assert path.form == ''
    assert path.basename is None
    assert path.dirname is None


@pytest.mark.parametrize('args', (('abc',), (u'абв',)), ids=('ascii', 'unicode'))
def test_path_single(args):
    path = lib.Path(*args)
    assert isinstance(path, tuple)
    assert isinstance(path, lib.Path)
    assert path == (args[0],)
    assert len(path) == 1
    assert path.form == args[0]
    assert path.basename == args[0]
    assert path.dirname == ()


def test_path():
    path = lib.Path('some/path')
    assert isinstance(path, tuple)
    assert isinstance(path, lib.Path)
    assert path == ('some', 'path')
    assert len(path) == 2
    assert path.form == 'some/path'
    assert path.basename == 'path'
    assert path.dirname == ('some',)


def test_path_norm():
    path = lib.Path('//some//path//')
    assert isinstance(path, tuple)
    assert isinstance(path, lib.Path)
    assert path == ('some', 'path')
    assert len(path) == 2
    assert path.form == 'some/path'
    assert path.basename == 'path'
    assert path.dirname == ('some',)


def test_is_subpath_of():
    assert lib.Path('/a/b').is_subpath_of(lib.Path('/a'))
    assert lib.Path('/a/b').is_subpath_of(lib.PathSplit.parse('/a'))
    assert lib.Path('/a/b').is_subpath_of('/a')

    assert not lib.Path('a/b').is_subpath_of(lib.Path('a/b_c'))
    assert not lib.Path('a/b').is_subpath_of(lib.PathSplit.parse('a/b_c'))
    assert not lib.Path('a/b').is_subpath_of('a/b_c')


@pytest.mark.parametrize('paths,common_prefix', [
    ((), ''),
    ((
        'some/path',
    ), 'some/path'),
    ((
        'some/path',
        'some/path/other/path',
        'some/path/other/path2',
        'some/path/subpath',
    ), 'some/path'),
    ((
        'some/path',
        'some/path/other/path',
        'some/path/other/path2',
        'some/path/subpath',
        'other/path',
    ), ''),
], ids=('empty', 'single', 'common_prefix', 'empty_common_prefix'))
def test_get_common_prefix(paths, common_prefix):
    assert lib.get_common_prefix(paths).form == common_prefix
