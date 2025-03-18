from __future__ import division, absolute_import, print_function, unicode_literals
import pytest
from statface_client.tools import NestedDict


def test_nested_dict():
    origin = {
        1: {
            10: {100: 'qqq'},
            11: {110: 'www', 111: {1110: 'eee', 1111: None}}
        },
        2: 'rrr'
    }
    check_state = repr(origin)
    matryoshka = NestedDict(origin)
    assert repr(origin) == repr(matryoshka)
    assert matryoshka is not origin

    updating = {1: {11: {111: {1111: 'gagaga'}}}}
    matryoshka.update(updating)
    assert repr(origin) == check_state
    assert repr(matryoshka) != repr(updating)
    assert repr(matryoshka) != repr(origin)
    assert matryoshka[1][11][111][1111] == 'gagaga'
    assert matryoshka[1][11][111][1110] == 'eee'
    assert matryoshka[1][11][110] == 'www'
    assert matryoshka[2] == 'rrr'

    with pytest.raises(KeyError):
        matryoshka['7']

    with pytest.raises(KeyError):
        matryoshka[1][11][112]

    try:
        matryoshka[1][11][111][234] = 'zuzuzu'
    except KeyError as e:
        assert str(e) == '234'

    with pytest.raises(AttributeError):
        del matryoshka[2]

    with pytest.raises(AttributeError):
        matryoshka.pop()
