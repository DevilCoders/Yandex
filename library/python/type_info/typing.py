from . import bindings
from . import extension

from .extension import Type, is_valid_type, _with_type, _is_utf8, validate_type

import six


class OptionalMeta(extension.SingleArgumentMeta):
    def make_cpp_type(cls, item):
        return bindings.make_optional(item)

    def __new__(cls, *args, **kwargs):
        return super(OptionalMeta, cls).__new__(cls, "Optional", *args, **kwargs)


class Optional(six.with_metaclass(OptionalMeta, type)):
    pass


class ListMeta(extension.SingleArgumentMeta):
    def make_cpp_type(cls, item):
        return bindings.make_list(item)

    def __new__(cls, *args, **kwargs):
        return super(ListMeta, cls).__new__(cls, "List", *args, **kwargs)


class List(six.with_metaclass(ListMeta, type)):
    pass


class DictMeta(extension.ComparableStrMeta):
    def __getitem__(self, params):
        if not isinstance(params, tuple) or len(params) != 2:
            raise ValueError("Expected two types in Dict, but got {}".format(_with_type(params)))
        key, value = params
        extension.validate_type(key)
        extension.validate_type(value)

        @classmethod
        def show(cls):
            return "Dict<{},{}>".format(cls.key, cls.value)
        attrs = {
            "key": key,
            "value": value,
            # "payload" is deprecated.
            "payload": value,
            "name": "Dict",
            "show": show,
        }
        if hasattr(key, "cpp_type") and hasattr(value, "cpp_type"):
            attrs["cpp_type"] = bindings.make_dict(key.cpp_type, value.cpp_type)
        return extension.ComparableStrMeta("Dict", (Type,), attrs)


class Dict(six.with_metaclass(DictMeta, type)):
    pass


def _make_tuple_attrs(items, name):
    for item in items:
        extension.validate_type(item)

    @classmethod
    def show(cls):
        return "{}<{}>".format(cls.name, ",".join(str(x) for x in cls.items))
    attrs = {
        "items": items,
        "name": name,
        "show": show,
    }
    if all(hasattr(item, "cpp_type") for item in items):
        attrs["cpp_type"] = bindings.make_tuple([item.cpp_type for item in items])
    return attrs


class TupleMeta(type):
    def __getitem__(self, params):
        if not isinstance(params, tuple):
            params = (params,)
        return extension.ComparableStrMeta("Tuple", (Type,), _make_tuple_attrs(params, "Tuple"))


class Tuple(six.with_metaclass(TupleMeta, type)):
    pass


def _check_struct_field_names(items):
    names = set()
    for name, item in items:
        if name == "":
            raise ValueError("Empty field name is not allowed")
        if name in names:
            raise ValueError("Duplicate fields are not allowed: " + name)
        names.add(name)


def _make_struct_attrs(params, name):
    for x in params:
        if not (
            isinstance(x, slice) and
            (isinstance(x.start, six.string_types) or isinstance(x.start, bytes)) and
            is_valid_type(x.stop) and
            x.step is None
        ):
            raise ValueError("Expected slice in form of field:type, but got {}".format(_with_type(x)))
    items = tuple((x.start, x.stop) for x in params)
    for item_name, type_ in items:
        if not _is_utf8(item_name):
            raise ValueError("Name of struct field must be UTF-8, got {}".format(_with_type(item_name)))
    _check_struct_field_names(items)

    @classmethod
    def show(cls):
        joined = ",".join(
            u"{}:{}".format(extension.quote_string(name), item)
            for name, item in cls.items
        )
        return u"{}<{}>".format(cls.name, joined)
    attrs = {
        "items": items,
        "name": name,
        "show": show,
    }
    if all(hasattr(field_t, "cpp_type") for n, field_t in items):
        attrs["cpp_type"] = bindings.make_struct([(n, field_t.cpp_type) for n, field_t in items])
    return attrs


class StructMeta(type):
    def __getitem__(self, params):
        if not isinstance(params, tuple):
            params = (params,)
        return extension.ComparableStrMeta("Struct", (Type,), _make_struct_attrs(params, "Struct"))


class Struct(six.with_metaclass(StructMeta, type)):
    pass


class VariantMeta(type):
    def __getitem__(self, params):
        if not isinstance(params, tuple):
            params = (params,)
        if isinstance(params[0], slice):
            attrs = _make_struct_attrs(params, "Variant")
            attrs["underlying"] = "Struct"
        else:
            attrs = _make_tuple_attrs(params, "Variant")
            attrs["underlying"] = "Tuple"
        if "cpp_type" in attrs:
            attrs["cpp_type"] = bindings.make_variant(attrs["cpp_type"])
        return extension.ComparableStrMeta("Variant", (Type,), attrs)


class Variant(six.with_metaclass(VariantMeta, type)):
    pass


class TaggedMeta(type):
    def __getitem__(self, params):
        if not (
            isinstance(params, tuple) and
            len(params) == 2 and
            is_valid_type(params[0]) and
            (isinstance(params[1], six.string_types) or isinstance(params[1], bytes))
        ):
            raise ValueError("Expected type and tag, but got {}".format(_with_type(params)))
        item, tag = params
        if not _is_utf8(tag):
            raise ValueError("Tag must be UTF-8, got {}".format(_with_type(tag)))

        @classmethod
        def show(cls):
            return u"Tagged<{},{}>".format(cls.item, extension.quote_string(cls.tag))
        attrs = {
            "item": item,
            "tag": tag,
            # "base" is deprecated.
            "base": item,
            "name": "Tagged",
            "show": show,
        }
        if hasattr(item, "cpp_type"):
            attrs["cpp_type"] = bindings.make_tagged(item.cpp_type, tag)
        return extension.ComparableStrMeta("Tagged", (Type,), attrs)


class Tagged(six.with_metaclass(TaggedMeta, type)):
    pass


class DecimalMeta(type):
    def _create(self, precision, scale):
        @classmethod
        def show(cls):
            return "Decimal({},{})".format(cls.precision, cls.scale)

        if not isinstance(precision, int):
            raise ValueError("Expected integer, but got {}".format(_with_type(precision)))
        if not isinstance(scale, int):
            raise ValueError("Expected integer, but got {}".format(_with_type(scale)))
        return extension.ComparableStrMeta("Decimal", (Type,), {
            "precision": precision,
            "scale": scale,
            "name": "Decimal",
            "show": show,
            "cpp_type": bindings.make_decimal(precision, scale),
        })

    def __call__(self, precision, scale):
        return self._create(precision, scale)

    def __getitem__(self, params):
        if not isinstance(params, tuple) or len(params) != 2:
            raise ValueError("Expected precision and scale integer parameters")
        precision, scale = params
        return self._create(precision, scale)


class Decimal(six.with_metaclass(DecimalMeta, type)):
    pass


def are_types_equal(type1, type2):
    validate_type(type1)
    validate_type(type2)
    return bindings.are_types_equal(type1, type2)

serialize_yson = bindings.serialize_yson
deserialize_yson = bindings.deserialize_yson

Bool = extension.make_primitive_type("Bool")
Int8 = extension.make_primitive_type("Int8")
Uint8 = extension.make_primitive_type("Uint8")
Int16 = extension.make_primitive_type("Int16")
Uint16 = extension.make_primitive_type("Uint16")
Int32 = extension.make_primitive_type("Int32")
Uint32 = extension.make_primitive_type("Uint32")
Int64 = extension.make_primitive_type("Int64")
Uint64 = extension.make_primitive_type("Uint64")
Float = extension.make_primitive_type("Float")
Double = extension.make_primitive_type("Double")
String = extension.make_primitive_type("String")
Utf8 = extension.make_primitive_type("Utf8")
Yson = extension.make_primitive_type("Yson")
Json = extension.make_primitive_type("Json")
Uuid = extension.make_primitive_type("Uuid")
Date = extension.make_primitive_type("Date")
Datetime = extension.make_primitive_type("Datetime")
Timestamp = extension.make_primitive_type("Timestamp")
Interval = extension.make_primitive_type("Interval")
TzDate = extension.make_primitive_type("TzDate")
TzDatetime = extension.make_primitive_type("TzDatetime")
TzTimestamp = extension.make_primitive_type("TzTimestamp")

Void = extension.make_primitive_type("Void")
Null = extension.make_primitive_type("Null")

EmptyTuple = Tuple.__getitem__(tuple())
EmptyStruct = Struct.__getitem__(tuple())
