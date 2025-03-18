from cython.operator cimport dereference
from util.generic.string cimport TString

from library.python.protobuf.dynamic_prototype.dynamic_prototype cimport (
    get_dynamic_message,
    Message,
    TDynamicMessage,
)
import six


cdef extern from "library/cpp/protobuf/json/json2proto.h" namespace "NProtobufJson" nogil:
    cdef void Json2Proto(const TString&, Message&, const TJson2ProtoConfig&) except +


cdef class Json2ProtoConfig:
    def __cinit__(
        self,
        field_name_mode=None,
        allow_unknown_fields=None,
        use_json_name=None,
        cast_from_string=None,
        do_not_cast_empty_strings=None,
        cast_robust=None,
        map_as_object=None,
        check_required_fields=None,
        replace_repeated_fields=None,
        enum_value_mode=None,
        vectorize_scalars=None,
    ):
        if field_name_mode is not None:
            self.base.FieldNameMode = field_name_mode
        if allow_unknown_fields is not None:
            self.base.AllowUnknownFields = allow_unknown_fields
        if use_json_name is not None:
            self.base.UseJsonName = use_json_name
        if cast_from_string is not None:
            self.base.CastFromString = cast_from_string
        if do_not_cast_empty_strings is not None:
            self.base.DoNotCastEmptyStrings = do_not_cast_empty_strings
        if cast_robust is not None:
            self.base.CastRobust = cast_robust
        if map_as_object is not None:
            self.base.MapAsObject = map_as_object
        if check_required_fields is not None:
            self.base.CheckRequiredFields = check_required_fields
        if replace_repeated_fields is not None:
            self.base.ReplaceRepeatedFields = replace_repeated_fields
        if enum_value_mode is not None:
            self.base.EnumValueMode = enum_value_mode
        if vectorize_scalars is not None:
            self.base.VectorizeScalars = vectorize_scalars


def json2proto(json, proto, config=Json2ProtoConfig()):
    cdef TDynamicMessage cpp_proto = get_dynamic_message(type(proto))

    Json2Proto(six.ensure_binary(json), dereference(cpp_proto.Get()), (<Json2ProtoConfig>config).base)

    proto.ParseFromString(dereference(cpp_proto.Get()).SerializeAsString())


def proto_from_json(json, proto_cls, config=Json2ProtoConfig()):
    proto = proto_cls()
    json2proto(json, proto, config)
    return proto
