from libcpp cimport bool


cdef extern from "library/cpp/protobuf/json/config.h" namespace "NProtobufJson::TProto2JsonConfig" nogil:
    cpdef enum MissingKeyMode:
        MissingKeySkip
        MissingKeyNull
        MissingKeyDefault
        MissingKeyExplicitDefaultThrowRequired

    cpdef enum EnumValueMode:
        EnumNumber
        EnumName
        EnumFullName
        EnumNameLowerCase
        EnumFullNameLowerCase

    cpdef enum FldNameMode:
        FieldNameOriginalCase
        FieldNameLowerCase
        FieldNameUpperCase
        FieldNameCamelCase
        FieldNameSnakeCase

    cpdef enum EStringifyLongNumbersMode:
        StringifyLongNumbersNever
        StringifyLongNumbersForFloat
        StringifyLongNumbersForDouble


cdef extern from "library/cpp/protobuf/json/config.h" namespace "NProtobufJson" nogil:
    cdef cppclass TProto2JsonConfig:
        bool FormatOutput

        MissingKeyMode MissingSingleKeyMode
        MissingKeyMode MissingRepeatedKeyMode

        bool AddMissingFields

        EnumValueMode EnumMode

        FldNameMode FieldNameMode

        bool UseJsonName
        bool MapAsObject

        EStringifyLongNumbersMode StringifyLongNumbers

        bool WriteNanAsString


cdef class Proto2JsonConfig:
    cdef TProto2JsonConfig base
