from libcpp cimport bool


cdef extern from "library/cpp/protobuf/json/json2proto.h" namespace "NProtobufJson::TJson2ProtoConfig" nogil:
    cpdef enum EnumValueMode:
        EnumCaseSensetive
        EnumCaseInsensetive

    cpdef enum FldNameMode:
        FieldNameOriginalCase
        FieldNameLowerCase
        FieldNameUpperCase
        FieldNameCamelCase
        FieldNameSnakeCase


cdef extern from "library/cpp/protobuf/json/json2proto.h" namespace "NProtobufJson" nogil:
    cdef cppclass TJson2ProtoConfig:
        FldNameMode FieldNameMode
        bool AllowUnknownFields
        bool UseJsonName
        bool CastFromString
        bool DoNotCastEmptyStrings
        bool CastRobust
        bool MapAsObject
        bool CheckRequiredFields
        bool ReplaceRepeatedFields
        EnumValueMode EnumValueMode
        bool VectorizeScalars


cdef class Json2ProtoConfig:
    cdef TJson2ProtoConfig base
