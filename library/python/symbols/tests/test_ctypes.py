import ctypes
import ctypes.util

import pytest


def test_libc():
    assert ctypes.CDLL(ctypes.util.find_library('c')).strtod
    assert ctypes.CDLL(ctypes.util.find_library('rt')).strtod
    assert ctypes.CDLL(ctypes.util.find_library('pthread')).strtod


def test_uuid():
    import uuid

    assert uuid._uuid_generate_time
    assert uuid.uuid1()


def test_exception():
    libc = ctypes.CDLL(ctypes.util.find_library('c'))
    with pytest.raises(AttributeError):
        libc.no_symbol_raises_attribute_error
