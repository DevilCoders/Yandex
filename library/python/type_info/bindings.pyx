from util.generic.array_ref cimport TConstArrayRef
from util.generic.maybe cimport TMaybe
from util.generic.ptr cimport TIntrusiveConstPtr
from util.generic.string cimport TStringBuf, TString
from util.generic.vector cimport TVector

from util.string.cast cimport ToString

from util.system.types cimport ui8

from libcpp cimport bool

import six


cdef extern from "library/cpp/type_info/type_list.h" namespace "NTi" nogil:
    cdef enum ETypeName:
        pass


cdef extern from "library/cpp/type_info/type.h" namespace "NTi" nogil:
    cdef cppclass TType:
        ETypeName GetTypeName() const

        bool IsVoid() const
        bool IsNull() const
        bool IsBool() const
        bool IsInt8() const
        bool IsInt16() const
        bool IsInt32() const
        bool IsInt64() const
        bool IsUint8() const
        bool IsUint16() const
        bool IsUint32() const
        bool IsUint64() const
        bool IsFloat() const
        bool IsDouble() const
        bool IsString() const
        bool IsUtf8() const
        bool IsDate() const
        bool IsDatetime() const
        bool IsTimestamp() const
        bool IsTzDate() const
        bool IsTzDatetime() const
        bool IsTzTimestamp() const
        bool IsInterval() const
        bool IsJson() const
        bool IsYson() const
        bool IsUuid() const

        bool IsOptional() const
        const TOptionalType* AsOptionalRaw() const
        bool IsList() const
        const TListType* AsListRaw() const
        bool IsDict() const
        const TDictType* AsDictRaw() const
        bool IsStruct() const
        const TStructType* AsStructRaw() const
        bool IsTuple() const
        const TTupleType* AsTupleRaw() const
        bool IsVariant() const
        const TVariantType* AsVariantRaw() const
        bool IsTagged() const
        const TTaggedType* AsTaggedRaw() const
        bool IsDecimal() const
        const TDecimalType* AsDecimalRaw() const
    ctypedef TIntrusiveConstPtr[TType] TTypePtr


cdef extern from "library/cpp/type_info/type.h" namespace "NTi::TTupleType":
    cdef cppclass TOwnedElement:
        TOwnedElement(TTypePtr type) except+
    cdef cppclass TElement:
        TTypePtr GetType() const
    ctypedef TConstArrayRef[TOwnedElement] TOwnedElements
    ctypedef TConstArrayRef[TElement] TElements


cdef extern from "library/cpp/type_info/type.h" namespace "NTi::TStructType":
    cdef cppclass TOwnedMember:
        TOwnedMember(TString name, const TTypePtr type) except+
    cdef cppclass TMember:
        TStringBuf GetName() const
        TTypePtr GetType() const
    ctypedef TConstArrayRef[TOwnedMember] TOwnedMembers
    ctypedef TConstArrayRef[TMember] TMembers


cdef extern from "library/cpp/type_info/type.h" namespace "NTi" nogil:
    cdef cppclass TVoidType:
        pass
    ctypedef TIntrusiveConstPtr[TVoidType] TVoidTypePtr

    cdef cppclass TNullType:
        pass
    ctypedef TIntrusiveConstPtr[TNullType] TNullTypePtr

    cdef cppclass TPrimitiveType:
        pass
    ctypedef TIntrusiveConstPtr[TPrimitiveType] TPrimitiveTypePtr

    cdef cppclass TBoolType:
        pass
    ctypedef TIntrusiveConstPtr[TBoolType] TBoolTypePtr

    cdef cppclass TInt8Type:
        pass
    ctypedef TIntrusiveConstPtr[TInt8Type] TInt8TypePtr

    cdef cppclass TInt16Type:
        pass
    ctypedef TIntrusiveConstPtr[TInt16Type] TInt16TypePtr

    cdef cppclass TInt32Type:
        pass
    ctypedef TIntrusiveConstPtr[TInt32Type] TInt32TypePtr

    cdef cppclass TInt64Type:
        pass
    ctypedef TIntrusiveConstPtr[TInt64Type] TInt64TypePtr

    cdef cppclass TUint8Type:
        pass
    ctypedef TIntrusiveConstPtr[TUint8Type] TUint8TypePtr

    cdef cppclass TUint16Type:
        pass
    ctypedef TIntrusiveConstPtr[TUint16Type] TUint16TypePtr

    cdef cppclass TUint32Type:
        pass
    ctypedef TIntrusiveConstPtr[TUint32Type] TUint32TypePtr

    cdef cppclass TUint64Type:
        pass
    ctypedef TIntrusiveConstPtr[TUint64Type] TUint64TypePtr

    cdef cppclass TFloatType:
        pass
    ctypedef TIntrusiveConstPtr[TFloatType] TFloatTypePtr

    cdef cppclass TDoubleType:
        pass
    ctypedef TIntrusiveConstPtr[TDoubleType] TDoubleTypePtr

    cdef cppclass TStringType:
        pass
    ctypedef TIntrusiveConstPtr[TStringType] TStringTypePtr

    cdef cppclass TUtf8Type:
        pass
    ctypedef TIntrusiveConstPtr[TUtf8Type] TUtf8TypePtr

    cdef cppclass TDateType:
        pass
    ctypedef TIntrusiveConstPtr[TDateType] TDateTypePtr

    cdef cppclass TDatetimeType:
        pass
    ctypedef TIntrusiveConstPtr[TDatetimeType] TDatetimeTypePtr

    cdef cppclass TTimestampType:
        pass
    ctypedef TIntrusiveConstPtr[TTimestampType] TTimestampTypePtr

    cdef cppclass TTzDateType:
        pass
    ctypedef TIntrusiveConstPtr[TTzDateType] TTzDateTypePtr

    cdef cppclass TTzDatetimeType:
        pass
    ctypedef TIntrusiveConstPtr[TTzDatetimeType] TTzDatetimeTypePtr

    cdef cppclass TTzTimestampType:
        pass
    ctypedef TIntrusiveConstPtr[TTzTimestampType] TTzTimestampTypePtr

    cdef cppclass TIntervalType:
        pass
    ctypedef TIntrusiveConstPtr[TIntervalType] TIntervalTypePtr

    cdef cppclass TDecimalType:
        ui8 GetPrecision() const
        ui8 GetScale() const
    ctypedef TIntrusiveConstPtr[TDecimalType] TDecimalTypePtr

    cdef cppclass TJsonType:
        pass
    ctypedef TIntrusiveConstPtr[TJsonType] TJsonTypePtr

    cdef cppclass TYsonType:
        pass
    ctypedef TIntrusiveConstPtr[TYsonType] TYsonTypePtr

    cdef cppclass TUuidType:
        pass
    ctypedef TIntrusiveConstPtr[TUuidType] TUuidTypePtr

    cdef cppclass TOptionalType:
        TTypePtr GetItemType() const
    ctypedef TIntrusiveConstPtr[TOptionalType] TOptionalTypePtr

    cdef cppclass TListType:
        TTypePtr GetItemType() const
    ctypedef TIntrusiveConstPtr[TListType] TListTypePtr

    cdef cppclass TDictType:
        TTypePtr GetKeyType() const
        TTypePtr GetValueType() const
    ctypedef TIntrusiveConstPtr[TDictType] TDictTypePtr

    cdef cppclass TStructType:
        TMembers GetMembers() const
    ctypedef TIntrusiveConstPtr[TStructType] TStructTypePtr

    cdef cppclass TTupleType:
        TElements GetElements() const
    ctypedef TIntrusiveConstPtr[TTupleType] TTupleTypePtr

    cdef cppclass TVariantType:
        TTypePtr GetUnderlyingType() const
    ctypedef TIntrusiveConstPtr[TVariantType] TVariantTypePtr

    cdef cppclass TTaggedType:
        TStringBuf GetTag() const
        TTypePtr GetItemType() const
    ctypedef TIntrusiveConstPtr[TTaggedType] TTaggedTypePtr


cdef extern from "library/cpp/type_info/type_equivalence.h" namespace "NTi::NEq":
    cdef cppclass TStrictlyEqual:
        TStrictlyEqual()
        bool operator()(TTypePtr lhs, TTypePtr rhs)


cdef extern from "library/cpp/type_info/type_constructors.h" namespace "NTi":
    TVoidTypePtr Void()
    TNullTypePtr Null()
    TBoolTypePtr Bool()
    TInt8TypePtr Int8()
    TInt16TypePtr Int16()
    TInt32TypePtr Int32()
    TInt64TypePtr Int64()
    TUint8TypePtr Uint8()
    TUint16TypePtr Uint16()
    TUint32TypePtr Uint32()
    TUint64TypePtr Uint64()
    TFloatTypePtr Float()
    TDoubleTypePtr Double()
    TStringTypePtr String()
    TUtf8TypePtr Utf8()
    TDateTypePtr Date()
    TDatetimeTypePtr Datetime()
    TTimestampTypePtr Timestamp()
    TTzDateTypePtr TzDate()
    TTzDatetimeTypePtr TzDatetime()
    TTzTimestampTypePtr TzTimestamp()
    TIntervalTypePtr Interval()
    TJsonTypePtr Json()
    TYsonTypePtr Yson()
    TUuidTypePtr Uuid()

    TDecimalTypePtr Decimal(ui8 precision, ui8 scale)
    TOptionalTypePtr Optional(TTypePtr)
    TListTypePtr List(TTypePtr)
    TDictTypePtr Dict(TTypePtr key, TTypePtr value)
    TStructTypePtr Struct(TStructType.TOwnedMembers members)
    TTupleTypePtr Tuple(TTupleType.TOwnedElements items)
    TVariantTypePtr Variant(TTypePtr underlying)
    TTaggedTypePtr Tagged(TTypePtr type, TStringBuf tag)


cdef extern from "library/cpp/type_info/type_factory.h" namespace "NTi" nogil:
    cdef cppclass ITypeFactory:
        pass
    ctypedef TIntrusiveConstPtr[ITypeFactory] ITypeFactoryPtr

    cdef ITypeFactoryPtr HeapFactory()


cdef extern from "library/cpp/type_info/type_io.h" namespace "NTi::NIo" nogil:
    TTypePtr DeserializeYson(ITypeFactory& factory, TStringBuf data, bool deduplicate) except+
    TString SerializeYson(const TType* type, bool humanReadable, bool includeTags) except+


cdef class WrappedType:
    cdef TTypePtr t


cdef inline wrap(TTypePtr t):
    wt = WrappedType()
    wt.t = t
    return wt

def make_bool():
    return wrap(<TTypePtr>Bool())
def make_int8():
    return wrap(<TTypePtr>Int8())
def make_uint8():
    return wrap(<TTypePtr>Uint8())
def make_int16():
    return wrap(<TTypePtr>Int16())
def make_uint16():
    return wrap(<TTypePtr>Uint16())
def make_int32():
    return wrap(<TTypePtr>Int32())
def make_uint32():
    return wrap(<TTypePtr>Uint32())
def make_int64():
    return wrap(<TTypePtr>Int64())
def make_uint64():
    return wrap(<TTypePtr>Uint64())
def make_float():
    return wrap(<TTypePtr>Float())
def make_double():
    return wrap(<TTypePtr>Double())
def make_string():
    return wrap(<TTypePtr>String())
def make_utf8():
    return wrap(<TTypePtr>Utf8())
def make_yson():
    return wrap(<TTypePtr>Yson())
def make_json():
    return wrap(<TTypePtr>Json())
def make_uuid():
    return wrap(<TTypePtr>Uuid())
def make_date():
    return wrap(<TTypePtr>Date())
def make_datetime():
    return wrap(<TTypePtr>Datetime())
def make_timestamp():
    return wrap(<TTypePtr>Timestamp())
def make_interval():
    return wrap(<TTypePtr>Interval())
def make_tz_date():
    return wrap(<TTypePtr>TzDate())
def make_tz_datetime():
    return wrap(<TTypePtr>TzDatetime())
def make_tz_timestamp():
    return wrap(<TTypePtr>TzTimestamp())

def make_void():
    return wrap(<TTypePtr>Void())
def make_null():
    return wrap(<TTypePtr>Null())

def make_optional(item):
    return wrap(<TTypePtr>Optional((<WrappedType>item).t))

def make_list(item):
    return wrap(<TTypePtr>List((<WrappedType>item).t))

def make_dict(key, value):
    return wrap(<TTypePtr>Dict((<WrappedType>key).t, (<WrappedType>value).t))

def make_tuple(items):
    cdef:
        TVector[TOwnedElement] v
        TMaybe[TOwnedElement] maybe
    for item in items:
        # Use TMaybe to circumvent TOwnedElement's lack of default constructor.
        maybe.ConstructInPlace((<WrappedType>item).t)
        v.push_back(maybe.GetRef())
    return wrap(<TTypePtr>Tuple(TOwnedElements(v)))

cdef TString to_bytes(s) except*:
    return <TString>six.ensure_binary(s)

def make_struct(items):
    cdef:
        TVector[TOwnedMember] v
        TMaybe[TOwnedMember] maybe
    for name, item in items:
        # Use TMaybe to circumvent TOwnedMember's lack of default constructor.
        maybe.ConstructInPlace(to_bytes(name), (<WrappedType>item).t)
        v.push_back(maybe.GetRef())
    return wrap(<TTypePtr>Struct(TOwnedMembers(v)))

def make_variant(item):
    return wrap(<TTypePtr>Variant((<WrappedType>item).t))

def make_tagged(item, tag):
    return wrap(<TTypePtr>Tagged((<WrappedType>item).t, to_bytes(tag)))

def make_decimal(precision, scale):
    return wrap(<TTypePtr>Decimal(<ui8>precision, <ui8>scale))

def are_equal(type1, type2):
    return TStrictlyEqual()((<WrappedType>type1).t, (<WrappedType>type2).t)

################################################################################

def serialize_yson(type_, human_readable=False):
    from . import typing
    if not typing.is_valid_type(type_):
        raise TypeError("serialize_yson can be called only for types")
    if not hasattr(type_, "cpp_type"):
        raise ValueError(
            "Cannot serialize type {} to YSON: it lacks \"cpp_type\" attribute"
            .format(type_)
        )
    cdef WrappedType wrapped = type_.cpp_type
    return bytes(SerializeYson(wrapped.t.Get(), human_readable, True))

def _cpp_to_python_type(wrapped):
    from . import typing
    type_ = (<WrappedType>wrapped).t.Get()
    if type_.IsVoid():
        return typing.Void
    elif type_.IsNull():
        return typing.Null
    elif type_.IsBool():
        return typing.Bool
    elif type_.IsInt8():
        return typing.Int8
    elif type_.IsInt16():
        return typing.Int16
    elif type_.IsInt32():
        return typing.Int32
    elif type_.IsInt64():
        return typing.Int64
    elif type_.IsUint8():
        return typing.Uint8
    elif type_.IsUint16():
        return typing.Uint16
    elif type_.IsUint32():
        return typing.Uint32
    elif type_.IsUint64():
        return typing.Uint64
    elif type_.IsFloat():
        return typing.Float
    elif type_.IsDouble():
        return typing.Double
    elif type_.IsString():
        return typing.String
    elif type_.IsUtf8():
        return typing.Utf8
    elif type_.IsDate():
        return typing.Date
    elif type_.IsDatetime():
        return typing.Datetime
    elif type_.IsTimestamp():
        return typing.Timestamp
    elif type_.IsTzDate():
        return typing.TzDate
    elif type_.IsTzDatetime():
        return typing.TzDatetime
    elif type_.IsTzTimestamp():
        return typing.TzTimestamp
    elif type_.IsInterval():
        return typing.Interval
    elif type_.IsJson():
        return typing.Json
    elif type_.IsYson():
        return typing.Yson
    elif type_.IsUuid():
        return typing.Uuid
    elif type_.IsOptional():
        item = type_.AsOptionalRaw().GetItemType()
        return typing.Optional[_cpp_to_python_type(wrap(item))]
    elif type_.IsList():
        item = type_.AsListRaw().GetItemType()
        return typing.List[_cpp_to_python_type(wrap(item))]
    elif type_.IsDict():
        key, value = type_.AsDictRaw().GetKeyType(), type_.AsDictRaw().GetValueType()
        return typing.Dict[_cpp_to_python_type(wrap(key)), _cpp_to_python_type(wrap(value))]
    elif type_.IsStruct():
        members = type_.AsStructRaw().GetMembers()
        slices = []
        for i in range(members.size()):
            member_type = _cpp_to_python_type(wrap(members[i].GetType()))
            slices.append(slice(six.ensure_text(members[i].GetName()), member_type))
        return typing.Struct[tuple(slices)]
    elif type_.IsTuple():
        elements = type_.AsTupleRaw().GetElements()
        py_elements = []
        for i in range(elements.size()):
            py_elements.append(_cpp_to_python_type(wrap(elements[i].GetType())))
        return typing.Tuple[tuple(py_elements)]
    elif type_.IsVariant():
        underlying = _cpp_to_python_type(wrap(type_.AsVariantRaw().GetUnderlyingType()))
        if underlying.name == "Tuple":
            return typing.Variant[tuple(underlying.items)]
        else:
            assert underlying.name == "Struct"
            return typing.Variant[tuple(slice(name, t) for name, t in underlying.items)]
    elif type_.IsTagged():
        item1 = _cpp_to_python_type(wrap(type_.AsTaggedRaw().GetItemType()))
        return typing.Tagged[item1, six.ensure_text(type_.AsTaggedRaw().GetTag())]
    elif type_.IsDecimal():
        precision, scale = type_.AsDecimalRaw().GetPrecision(), type_.AsDecimalRaw().GetScale()
        return typing.Decimal[precision, scale]
    else:
        type_name = ToString(type_.GetTypeName())
        raise ValueError("Cannot convert C++ type {} to python type".format(type_name))

def deserialize_yson(yson):
    cdef ITypeFactory* factory = HeapFactory().Get()
    try:
        return _cpp_to_python_type(wrap(<TTypePtr>DeserializeYson(factory[0], to_bytes(yson), True)))
    except RuntimeError as error:
        raise ValueError("Deserialization failed: " + str(error))
