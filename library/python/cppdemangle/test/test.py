import pytest
from library.python.cppdemangle import cppdemangle, InvalidMangledName


def test_demangle():
    assert cppdemangle('f') == 'f'
    assert cppdemangle('_Z1fv') == 'f()'
    assert cppdemangle('__Z1fv') == 'f()'
    assert cppdemangle('?f@@YAXXZ') == 'void __cdecl f(void)'


def test_fail():
    with pytest.raises(InvalidMangledName):
        cppdemangle('_Zf')
