import pytest

import library.python.path.split as path_split


def test_empty():
    split = path_split.Split()
    assert isinstance(split, tuple)
    assert split == ()
    assert len(split) == 0
    assert split.form == ''
    assert split.basename is None
    assert split.dirname is None
    assert path_split.Split.parse('') == split
    assert path_split.Split.parse(u'') == split
    assert path_split.Split.parse('/') == split
    assert path_split.Split.parse(u'/') == split


def test_single():
    split = path_split.Split(('abc',))
    assert isinstance(split, tuple)
    assert split == ('abc',)
    assert len(split) == 1
    assert split.form == 'abc'
    assert split.basename == 'abc'
    assert split.dirname == ()
    assert path_split.Split.parse('abc') == split
    assert path_split.Split.parse(u'abc') == split
    assert path_split.Split.parse('/abc') == split
    assert path_split.Split.parse(u'/abc') == split


def test_single_unicode():
    split = path_split.Split((u'абв',))
    assert isinstance(split, tuple)
    assert split == (u'абв',)
    assert len(split) == 1
    assert split.form == u'абв'
    assert split.basename == u'абв'
    assert split.dirname == ()
    assert path_split.Split.parse(u'абв') == split
    assert path_split.Split.parse(u'/абв') == split


def test_usual():
    split = path_split.Split(('some', 'path'))
    assert isinstance(split, tuple)
    assert split == ('some', 'path')
    assert len(split) == 2
    assert split.form == 'some/path'
    assert split.basename == 'path'
    assert split.dirname == ('some',)
    assert path_split.Split.parse('some/path') == split
    assert path_split.Split.parse('/some/path') == split


def test_parse_norm():
    assert path_split.Split.parse('///') == ()
    assert path_split.Split.parse('abc/') == ('abc',)
    assert path_split.Split.parse('//abc//def//') == ('abc', 'def')


def test_is_subpath_of():
    assert path_split.Split.parse('/a/b').is_subpath_of(path_split.Split.parse('/a'))
    assert path_split.Split.parse('/a/b').is_subpath_of(path_split.Split.parse('/a/'))
    assert path_split.Split.parse('/a/b/').is_subpath_of(path_split.Split.parse('/a'))
    assert path_split.Split.parse('a/b').is_subpath_of(path_split.Split.parse('a'))
    assert path_split.Split.parse('a/b').is_subpath_of(path_split.Split.parse('a/'))
    assert path_split.Split.parse('a/b/').is_subpath_of(path_split.Split.parse('a'))

    assert not path_split.Split.parse('a/b').is_subpath_of(path_split.Split.parse('a/b_c'))


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
    cp = path_split.get_common_prefix(path_split.Split.parse(path) for path in paths)
    assert cp.form == common_prefix
