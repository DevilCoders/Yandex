from . import bindings

import six


def _with_type(x):
    return "<type: {!s}>: {!r}".format(type(x), x)


def _is_utf8(s):
    if not isinstance(s, six.string_types):
        return False
    try:
        six.ensure_text(s)
        return True
    except UnicodeDecodeError:
        return False


class DoNotCallMeta(type):
    def __call__(cls, *args):
        raise TypeError(
            "Using '()' is not allowed; maybe you meant '[]': {type}[{args}]?".format(
                type=cls,
                args=", ".join(map(repr, args)),
            )
        )


class Type(six.with_metaclass(DoNotCallMeta, type)):
    pass


def is_valid_type(x):
    return isinstance(x, type) and issubclass(x, Type)


def validate_type(x):
    if not is_valid_type(x):
        raise ValueError("Expected type, but got <type: {!s}>: {!r}".format(type(x), x))


def quote_string(s):
    return u"'{}'".format(s.replace("\\", "\\\\").replace("'", "\\'"))


_PRIMITIVE_TYPES = {
    "Bool": bindings.make_bool(),
    "Int8": bindings.make_int8(),
    "Uint8": bindings.make_uint8(),
    "Int16": bindings.make_int16(),
    "Uint16": bindings.make_uint16(),
    "Int32": bindings.make_int32(),
    "Uint32": bindings.make_uint32(),
    "Int64": bindings.make_int64(),
    "Uint64": bindings.make_uint64(),
    "Float": bindings.make_float(),
    "Double": bindings.make_double(),
    "String": bindings.make_string(),
    "Utf8": bindings.make_utf8(),
    "Yson": bindings.make_yson(),
    "Json": bindings.make_json(),
    "Uuid": bindings.make_uuid(),
    "Date": bindings.make_date(),
    "Datetime": bindings.make_datetime(),
    "Timestamp": bindings.make_timestamp(),
    "Interval": bindings.make_interval(),
    "TzDate": bindings.make_tz_date(),
    "TzDatetime": bindings.make_tz_datetime(),
    "TzTimestamp": bindings.make_tz_timestamp(),
    "Void": bindings.make_void(),
    "Null": bindings.make_null(),
}


def make_primitive_type(name, show=None):
    assert _is_utf8(name), "Name of primitive type must be UTF-8, got {}".format(_with_type(name))

    @classmethod
    def _show(cls):
        return cls.name if show is None else show()
    attrs = {
        "name": name,
        "show": _show,
    }
    if name in _PRIMITIVE_TYPES:
        attrs["cpp_type"] = _PRIMITIVE_TYPES[name]
    return StrMeta(name, (Type,), attrs)


@six.python_2_unicode_compatible
class StrMeta(DoNotCallMeta):
    def __str__(cls):
        return cls.show()


class ComparableStrMeta(StrMeta):
    def __eq__(self, other):
        if hasattr(self, "cpp_type") != hasattr(other, "cpp_type"):
            return False
        if not hasattr(self, "cpp_type") and not hasattr(other, "cpp_type"):
            raise ValueError("Cannot compare types without \"cpp_type\" attribute: {} and {}".format(self, other))
        return bindings.are_equal(self.cpp_type, other.cpp_type)

    def __neq__(self, other):
        return not(self == other)


class SingleArgumentMeta(DoNotCallMeta):
    def __new__(cls, name, *args, **kwargs):
        assert _is_utf8(name), "Name must be UTF-8, got {}".format(_with_type(name))
        cls.name = name
        return super(SingleArgumentMeta, cls).__new__(cls, *args, **kwargs)

    def __getitem__(self, item):
        validate_type(item)

        @classmethod
        def show(cls):
            return "{}<{}>".format(cls.name, cls.item)
        attrs = {
            "item": item,
            "name": self.name,
            "show": show,
        }
        if hasattr(self, "make_cpp_type") and hasattr(item, "cpp_type"):
            attrs["cpp_type"] = self.make_cpp_type(item.cpp_type)
        return ComparableStrMeta(self.name, (Type,), attrs)
