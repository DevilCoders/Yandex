import ipaddress
import pytest

from schematics.types import StringType, ListType, IntType, ModelType, DictType
from schematics.types.serializable import serializable
from schematics.transforms import whitelist
from schematics.exceptions import DataError
from yc_common.models import (
    Model, IsoTimestampType, JsonListType, JsonReadListType,
    jsonschema_for_model, _format_schematics_error,
)
from yc_common.fields import IPAddressType, IPNetworkType


def test_primitive_field():
    class TestModel(Model):
        str = StringType()
        int = IntType()

    assert jsonschema_for_model(TestModel) == {
        "type": "object",
        "title": "test_primitive_field.<locals>.TestModel",
        "properties": {
            "str": {
                "type": "string",
            },
            "int": {
                "type": "integer",
            }
        }
    }


def test_object_field():
    class ObjectType(Model):
        str = StringType()

    class TestModel(Model):
        obj = ModelType(ObjectType)

    assert jsonschema_for_model(TestModel) == {
        "type": "object",
        "title": "test_object_field.<locals>.TestModel",
        "properties": {
            "obj": {
                "type": "object",
                "title": "test_object_field.<locals>.ObjectType",
                "properties": {
                    "str": {
                        "type": "string"
                    }
                }
            }
        }
    }


def test_list_of_primitives_field():
    class TestModel(Model):
        list = ListType(StringType())

    assert jsonschema_for_model(TestModel) == {
        "type": "object",
        "title": "test_list_of_primitives_field.<locals>.TestModel",
        "properties": {
            "list": {
                "type": "array",
                "items": {
                    "type": "string",
                }
            }
        }
    }


def test_list_of_objects_field():
    class ObjectType(Model):
        str = StringType()

    class TestModel(Model):
        list = ListType(ModelType(ObjectType))

    assert jsonschema_for_model(TestModel) == {
        "type": "object",
        "title": "test_list_of_objects_field.<locals>.TestModel",
        "properties": {
            "list": {
                "type": "array",
                "items": {
                    "type": "object",
                    "title": "test_list_of_objects_field.<locals>.ObjectType",
                    "properties": {
                        "str": {
                            "type": "string",
                        }
                    }
                }
            }
        }
    }


def test_dict_of_primitives_field():
    class TestModel(Model):
        dct = DictType(StringType)

    assert jsonschema_for_model(TestModel) == {
        "type": "object",
        "title": "test_dict_of_primitives_field.<locals>.TestModel",
        "properties": {
            "dct": {
                "type": "object",
                "additionalProperties": {
                    "type": "string",
                }
            }
        }
    }


def test_dict_of_objects_field():
    class ObjectType(Model):
        str = StringType()

    class TestModel(Model):
        dct = DictType(ModelType(ObjectType))

    assert jsonschema_for_model(TestModel) == {
        "type": "object",
        "title": "test_dict_of_objects_field.<locals>.TestModel",
        "properties": {
            "dct": {
                "type": "object",
                "additionalProperties": {
                    "type": "object",
                    "title": "test_dict_of_objects_field.<locals>.ObjectType",
                    "properties": {
                        "str": {
                            "type": "string",
                        }
                    }
                }

            }
        }
    }


def test_public_api_filtering():
    class ObjectType(Model):
        str1 = StringType()
        str2 = StringType()

        class Options:
            roles = {"public_api": whitelist("str1")}

    class TestModel(Model):
        obj1 = ModelType(ObjectType)
        obj2 = ModelType(ObjectType)

        class Options:
            roles = {"public_api": whitelist("obj1")}

    assert jsonschema_for_model(TestModel, public=True) == {
        "type": "object",
        "title": "test_public_api_filtering.<locals>.TestModel",
        "properties": {
            "obj1": {
                "type": "object",
                "title": "test_public_api_filtering.<locals>.ObjectType",
                "properties": {
                    "str1": {
                        "type": "string",
                    }
                }
            }
        }
    }


def test_required_field():
    class TestModel(Model):
        str1 = StringType(required=True)
        str2 = StringType()

    assert jsonschema_for_model(TestModel) == {
        "type": "object",
        "title": "test_required_field.<locals>.TestModel",
        "properties": {
            "str1": {
                "type": "string",
            },
            "str2": {
                "type": "string",
            }
        },
        "required": ["str1"],
    }


def test_computed_field():
    class TestModel(Model):
        str1 = StringType(required=True)

        @serializable(type=StringType)
        def str2(self):
            return 'foo'

    assert jsonschema_for_model(TestModel) == {
        "type": "object",
        "title": "test_computed_field.<locals>.TestModel",
        "properties": {
            "str1": {
                "type": "string",
            },
            "str2": {
                "type": "string",
            }
        },
        "required": ["str1"],
    }


def test_isotimestamp_native_notz():
    class M(Model):
        ts = IsoTimestampType()

    m = M({"ts": "2017-01-04T10:00:00"})
    assert m.ts == 1483524000


def test_isotimestamp_native_tz():
    class M(Model):
        ts = IsoTimestampType()

    m = M({"ts": "2017-12-07T10:00:00+03:00"})
    assert m.ts == 1512630000


def test_isotimestamp_primitive():
    class M(Model):
        ts = IsoTimestampType()

    m = M.new(ts=1512630000)
    assert m.to_primitive() == {"ts": "2017-12-07T07:00:00+00:00"}


def test_isotimestamp_bad_convert():
    class M(Model):
        ts = IsoTimestampType()

    with pytest.raises(DataError):
        M({"ts": [1512630000]})


def test_isotimestamp_bad_format():
    class M(Model):
        ts = IsoTimestampType()

    with pytest.raises(DataError):
        M({"ts": "unknown"})


def test_format_schematics_error():
    class _S(Model):
        required_field = StringType(serialized_name="requiredField", required=True)

    class _M(Model):
        required_field = StringType(serialized_name="requiredField", required=True)
        optional_field1 = IntType(serialized_name="optionalField1", required=False, max_value=3)
        optional_field2 = IntType(serialized_name="optionalField2", required=False, max_value=3)
        sub_field = ModelType(_S)

    with pytest.raises(DataError) as ex:
        request = {
            "unknownField": "value",
            "sub_field": {"unknownSubField": "value"},
            "optionalField1": "abc",
            "optionalField2": "100",
        }
        _M(request, validate=True, partial=False)

    message = _format_schematics_error(ex.value)
    expected = {
        "optionalField1: Value 'abc' is not int",
        "requiredField: This field is required",
        "unknownField: Unknown field",
        "sub_field.requiredField: This field is required",
        "sub_field.unknownSubField: Unknown field",
        "optionalField2: Int value should be less than or equal to 3"
    }

    assert set(message.split("; ")) == expected


def test_validate_address_success():
    class M(Model):
        address = IPAddressType(required=True)

    M({"address": "192.0.2.1"}, validate=True)
    M({"address": "2001:db8::cafe:bad:f00d"}, validate=True)


def test_validate_address_bad_parse():
    class M(Model):
        address = IPAddressType(required=True)

    with pytest.raises(DataError) as ex:
        M({"address": "192.0.300.1"}, validate=True)
    assert ex.value.errors["address"][0].summary.startswith("'192.0.300.1' is not a valid IP address")

    with pytest.raises(DataError) as ex:
        M({"address": "2001:db8::cafe:feffe:f00d"}, validate=True)
    assert ex.value.errors["address"][0].summary.startswith("'2001:db8::cafe:feffe:f00d' is not a valid IP address")


def test_validate_network_success():
    class M(Model):
        network = IPNetworkType(required=True)

    M({"network": "192.0.2.0/24"}, validate=True)
    M({"network": "2001:db8::/64"}, validate=True)


def test_validate_network_bad_parse():
    class M(Model):
        network = IPNetworkType(required=True)

    with pytest.raises(DataError) as ex:
        M({"network": "192.0.2.1/24"}, validate=True)
    assert ex.value.errors["network"][0].summary.startswith("'192.0.2.1/24' is not a valid IP network")

    with pytest.raises(DataError) as ex:
        M({"network": "2001:db8::cafe:bad:f00d/24"}, validate=True)
    assert ex.value.errors["network"][0].summary.startswith("'2001:db8::cafe:bad:f00d/24' is not a valid IP network")


def test_validate_address_version():
    class M(Model):
        v4_network = IPNetworkType(version=4, required=True)
        v6_network = IPNetworkType(version=6, required=True)

    M({"v4_network": "192.0.2.0/24",
       "v6_network": "2001:db8::/64"}, validate=True)

    with pytest.raises(DataError) as ex:
        M({"v6_network": "192.0.2.0/24",
           "v4_network": "2001:db8::/64"}, validate=True)
    assert ex.value.errors["v6_network"][0].summary.startswith("'192.0.2.0/24' is not a valid IPv6 network")
    assert ex.value.errors["v4_network"][0].summary.startswith("'2001:db8::/64' is not a valid IPv4 network")


def test_validate_network_prefix():
    class M(Model):
        v4_network = IPNetworkType(version=4, max_prefix=16, min_prefix=24, required=True)
        v6_network = IPNetworkType(version=6, max_prefix=64, min_prefix=120, required=True)

    M({"v4_network": "192.0.0.0/24",
       "v6_network": "2001:db8::/64"}, validate=True)

    with pytest.raises(DataError) as ex:
        M({"v4_network": "192.0.0.0/12",
           "v6_network": "2001:db8::/56"}, validate=True)
    assert ex.value.errors["v4_network"][0].summary.startswith("'192.0.0.0/12' value prefix length is greater than 16")
    assert ex.value.errors["v6_network"][0].summary.startswith("'2001:db8::/56' value prefix length is greater than 64")

    with pytest.raises(DataError) as ex:
        M({"v4_network": "192.0.0.0/29",
           "v6_network": "2001:db8::/126"}, validate=True)
    assert ex.value.errors["v4_network"][0].summary.startswith("'192.0.0.0/29' value prefix length is less than 24")
    assert ex.value.errors["v6_network"][0].summary.startswith("'2001:db8::/126' value prefix length is less than 120")


@pytest.mark.parametrize("address, network", [
    ("10.20.30.40", "10.20.30.0/24"),
    ("192.168.1.2", "192.168.1.0/24"),
    ("fcdb:cafe::bad:f00d", "fcdb:cafe::/64"),
    ("fc00:db8::cafe:bad:f00d", "fc00::/10"),
])
def test_validate_address_network_values_success(address, network):
    valid_values = [
        ipaddress.ip_network("10.0.0.0/8"),
        ipaddress.ip_network("172.16.0.0/12"),
        ipaddress.ip_network("192.168.0.0/16"),
        ipaddress.ip_network("fc00::/7"),
        ipaddress.ip_network("fe80::/10")
    ]

    class M(Model):
        address = IPAddressType(valid_values=valid_values, required=True)
        network = IPNetworkType(valid_values=valid_values, required=True)

    M({"address": address, "network": network}, validate=True)


@pytest.mark.parametrize("address, network", [
    ("192.0.1.2", "192.0.1.0/24"),
    ("ff02::1", "ff02::/64"),
])
def test_validate_address_network_values_error(address, network):
    valid_values = [
        ipaddress.ip_network("10.0.0.0/8"),
        ipaddress.ip_network("172.16.0.0/12"),
        ipaddress.ip_network("192.168.0.0/16"),
        ipaddress.ip_network("fc00::/7"),
        ipaddress.ip_network("fe80::/10")
    ]

    class M(Model):
        address = IPAddressType(valid_values=valid_values, required=True)
        network = IPNetworkType(valid_values=valid_values, required=True)

    with pytest.raises(DataError) as ex:
        M({"address": address,
           "network": network}, validate=True)
    assert ex.value.errors["address"][0].summary.startswith("'{}' value does not belong to allowed range '10.0.0.0/8, 172.16.0.0/12, 192.168.0.0/16, fc00::/7, fe80::/10'".format(address))
    assert ex.value.errors["network"][0].summary.startswith("'{}' value does not belong to allowed range '10.0.0.0/8, 172.16.0.0/12, 192.168.0.0/16, fc00::/7, fe80::/10'".format(network))


def test_json_list():
    class M(Model):
        L = JsonListType(StringType)

    # read as list
    m = M.from_kikimr({"L": ['a']})
    assert m.L == ['a']
    assert m.to_kikimr() == {"L": '["a"]'}

    # read as string
    m = M.from_kikimr({"L": '["a"]'})
    assert m.L == ['a']
    assert m.to_kikimr() == {"L": '["a"]'}


def test_json_read_list():
    class M(Model):
        L = JsonReadListType(StringType)

    # read as list
    m = M.from_kikimr({"L": ['a']})
    assert m.L == ['a']
    assert m.to_kikimr() == {"L": ["a"]}

    # read as string
    m = M.from_kikimr({"L": '["a"]'})
    assert m.L == ['a']
    assert m.to_kikimr() == {"L": ["a"]}
