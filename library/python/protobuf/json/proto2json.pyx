from cython.operator cimport dereference
from util.generic.string cimport TString

from library.python.protobuf.dynamic_prototype.dynamic_prototype cimport (
    get_dynamic_message,
    Message,
    TDynamicMessage,
)
import six


cdef extern from "library/cpp/protobuf/json/proto2json.h" namespace "NProtobufJson" nogil:
    cdef TString Proto2Json(const Message&, const TProto2JsonConfig&);


cdef class Proto2JsonConfig:
    def __cinit__(
        self,
        format_output=None,
        missing_single_key_mode=None,
        missing_repeated_key_mode=None,
        add_missing_fields=None,
        enum_mode=None,
        field_name_mode=None,
        use_json_name=None,
        map_as_object=None,
        stringify_long_numbers=None,
        write_nan_as_string=None,
    ):
        if format_output is not None:
            self.base.FormatOutput = format_output
        if missing_single_key_mode is not None:
            self.base.MissingSingleKeyMode = missing_single_key_mode
        if missing_repeated_key_mode is not None:
            self.base.MissingRepeatedKeyMode = missing_repeated_key_mode
        if add_missing_fields is not None:
            self.base.AddMissingFields = add_missing_fields
        if enum_mode is not None:
            self.base.EnumMode = enum_mode
        if field_name_mode is not None:
            self.base.FieldNameMode = field_name_mode
        if use_json_name is not None:
            self.base.UseJsonName = use_json_name
        if map_as_object is not None:
            self.base.MapAsObject = map_as_object
        if stringify_long_numbers is not None:
            self.base.StringifyLongNumbers = stringify_long_numbers
        if write_nan_as_string is not None:
            self.base.WriteNanAsString = write_nan_as_string


def proto2json(proto, config=Proto2JsonConfig()):
    return Proto2JsonConverter(type(proto), config).convert(proto)


cdef class Proto2JsonConverter(object):
    cdef TDynamicMessage cpp_proto
    cdef Proto2JsonConfig config

    def __init__(self, proto_cls, config=Proto2JsonConfig()):
        pass

    def __cinit__(self, proto_cls, config=Proto2JsonConfig()):
        self.cpp_proto = get_dynamic_message(proto_cls)
        self.config = config

    def convert(self, proto):
        if not dereference(self.cpp_proto.Get()).ParseFromString(proto.SerializeToString()):
            raise Exception("Failed to transfer protobuf from Python to C++")

        return six.ensure_str(Proto2Json(dereference(self.cpp_proto.Get()), (<Proto2JsonConfig>self.config).base))
