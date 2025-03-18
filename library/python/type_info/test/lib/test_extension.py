import pytest
import six

import yandex.type_info.typing as typing

from yandex.type_info.extension import (
    is_valid_type, validate_type, quote_string, make_primitive_type,
    StrMeta, SingleArgumentMeta,
)


def test_valid_type():
    assert is_valid_type(typing.Type)
    assert not is_valid_type(int)
    assert not is_valid_type(3)
    assert is_valid_type(typing.List[typing.Int8])


def test_check_type():
    validate_type(typing.Type)
    validate_type(typing.Int64)
    with pytest.raises(ValueError):
        validate_type(int)
    with pytest.raises(ValueError):
        validate_type(3)
    validate_type(typing.List[typing.Int8])


def test_quoute_string():
    assert quote_string("a'b'c'd") == "'a\\'b\\'c\\'d'"
    assert quote_string("abc") == "'abc'"
    assert quote_string("") == "''"
    assert quote_string("\\bcd\\") == "'\\\\bcd\\\\'"
    assert quote_string("\\b'c'd\\") == "'\\\\b\\'c\\'d\\\\'"
    assert quote_string(u"хэ\\л'ло") == u"'хэ\\\\л\\'ло'"


def test_make_primitive_type():
    MyType = make_primitive_type("MyTypeName")
    assert is_valid_type(MyType)
    assert str(MyType) == "MyTypeName"
    assert MyType.name == "MyTypeName"
    # Check no exception is thrown
    ListOfMyType = typing.List[MyType]
    assert is_valid_type(ListOfMyType)


def test_dont_call_meta():
    class MyType(six.with_metaclass(StrMeta, typing.Type)):
        @classmethod
        def show(cls):
            return "Muhaha"
    with pytest.raises(TypeError):
        MyType(typing.Int32)


def test_str_meta():
    class MyType(six.with_metaclass(StrMeta, typing.Type)):
        @classmethod
        def show(cls):
            return "Muhaha"
    assert str(MyType) == "Muhaha"


def test_single_argument_meta():
    class MyMeta(SingleArgumentMeta):
        def __new__(cls, *args, **kwargs):
            return super(MyMeta, cls).__new__(cls, "MyMeta", *args, **kwargs)

    class MyType(six.with_metaclass(MyMeta, type)):
        pass

    NewType = MyType[typing.Int32]
    assert is_valid_type(NewType)
