#pragma once

// Methods for working with erf records with statically known structure.
// Copyright (c) 2011 Yandex, LLC. All rights reserved.

#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/generic/strbuf.h>
#include <utility>

class IErfFieldsVisitor {
public:
    virtual ~IErfFieldsVisitor() {}
    virtual void VisitCppField(const char* type, const char* name, const void* value, size_t size);
};


namespace NErf {

// A signature consistent with TStaticReflection<TStruct>::SIGNATURE
ui32 GetRecordSignature(const TStringBuf& name);

// Kept POD for faster compile times of large codegenerated constant arrays of these items, see @MakeFieldDescriptor
// WARNING: THE ORDER OF FIELDS IS IMPORTANT (known usage: codegen_tool and @MakeFieldDescriptor)
struct TStaticFieldDescriptor {
    const char* const Name;
    const char* const Type;
    const ui32 Offset;
    const ui32 Width;
    const char* const PrevNames; // comma-separated, if any

    ui32 MaxValue() const {
        Y_VERIFY(Width <= 32, "Width must be not greater than 32.");
        return ui32(ui64(1) << Width) - 1;
    }
};


inline TStaticFieldDescriptor MakeFieldDescriptor(
    const ui32 offset,
    const ui32 width,
    const char* name = nullptr,
    const char* type = nullptr,
    const char* prevNames = nullptr)
{
    TStaticFieldDescriptor descriptor = { name, type, offset, width, prevNames };
    return descriptor;
}


struct TStaticRecordDescriptor {
    const char* const Name;
    const size_t Version;
    const size_t Size;
    const TStaticFieldDescriptor* const Fields;
    const size_t FieldCount;

    TStaticRecordDescriptor(
        const char* name,
        size_t version,
        size_t size,
        const TStaticFieldDescriptor* fields,
        size_t fieldCount
    )
        : Name(name)
        , Version(version)
        , Size(size)
        , Fields(fields)
        , FieldCount(fieldCount)
    {
    }
};


class TFieldReflection {
private:
    inline static ui32 DWordOffset(const ui32 bitOffset) {
        return (bitOffset / 32) * 4;
    }

    inline static ui32 ShiftInsideDWord(const ui32 bitOffset) {
        return bitOffset % 32;
    }

    static const ui32 MASKS[33];

    const TStaticFieldDescriptor& FieldDescriptor;

public:

    inline TFieldReflection(const TStaticFieldDescriptor& fieldDesctiptor)
        : FieldDescriptor(fieldDesctiptor)
    {
    }


    inline bool IsBitField() const {
        return FieldDescriptor.Type == nullptr;
    }

    inline ui32 GetBitField(const void* src) const noexcept {
        Y_ASSERT(IsBitField());
        const TStaticFieldDescriptor& f = FieldDescriptor;
        const ui32 dword = *(const ui32*)((const char*)src + DWordOffset(f.Offset));
        return (dword >> ShiftInsideDWord(f.Offset)) & MASKS[f.Width];
    }

    inline void SetBitField(void* dst, ui32 value) const noexcept {
        Y_ASSERT(IsBitField());
        const TStaticFieldDescriptor& f = FieldDescriptor;
        ui32& dword = *(ui32*)((char*)dst + DWordOffset(f.Offset));
        ui32 mask = MASKS[f.Width];
        ui32 shift = ShiftInsideDWord(f.Offset);
        dword = (dword & ~(mask << shift)) | ((value & mask) << shift);
    }

    inline void CopyBitField(void* dst, const void* src) const noexcept {
        Y_ASSERT(IsBitField());
        const TStaticFieldDescriptor& f = FieldDescriptor;
        const ui32 dwordSrc = *(const ui32*)((const char*)src + DWordOffset(f.Offset));
        ui32& dword = *(ui32*)((char*)dst + DWordOffset(f.Offset));
        ui32 mask = MASKS[f.Width] << ShiftInsideDWord(f.Offset);
        dword = (dword & ~mask) | (dwordSrc & mask);
    }


    // Methods for manipulating C++ fields in records

    ui32 GetCppFieldSize() const noexcept {
        Y_ASSERT(!IsBitField());
        return FieldDescriptor.Width / 8;
    }

    inline const void* GetCppFieldPtr(const void* src) const noexcept {
        Y_ASSERT(!IsBitField());
        return (const char*)src + FieldDescriptor.Offset / 8;
    }

    inline void* GetCppFieldPtr(void* src) const noexcept {
        Y_ASSERT(!IsBitField());
        return (char*)src + FieldDescriptor.Offset / 8;
    }

    inline void CopyCppField(void* dst, const void* src) const noexcept {
        Y_ASSERT(!IsBitField());
        memcpy(GetCppFieldPtr(dst), GetCppFieldPtr(src), GetCppFieldSize());
    }

    template<typename T>
    inline const T& GetCppField(const void* src) const noexcept {
        Y_ASSERT(GetCppFieldSize() == sizeof(T));
        return *static_cast<const T*>(GetCppFieldPtr(src));
    }

    template<typename T>
    inline void SetCppField(void* dst, const T& value) const noexcept {
        Y_ASSERT(GetCppFieldSize() == sizeof(T));
        memcpy(GetCppFieldPtr(dst), &value, sizeof(T));
    }


    // Generic field methods

    inline void ClearField(void* dst) const noexcept {
        if (Y_LIKELY(IsBitField()))
            SetBitField(dst, 0);
        else
            memset(GetCppFieldPtr(dst), 0, GetCppFieldSize());
    }

    inline void CopyField(void* dst, const void* src) const noexcept {
        if (Y_LIKELY(IsBitField()))
            CopyBitField(dst, src);
        else
            CopyCppField(dst, src);
    }

    template<typename T>
    inline T GetFieldUniversal(const void* src) const noexcept {
        if (Y_LIKELY(IsBitField())) {
            return GetBitField(src);
        } else {
            return GetCppField<T>(src);
        }
    }

    template<typename T>
    inline void SetFieldUniversal(void* dst, const T& value) const noexcept {
        if (Y_LIKELY(IsBitField())) {
            SetBitField(dst, value);
        } else {
            SetCppField(dst, value);
        }
    }
};


class TNameToIndexMap;

// Represents a sequence of erf fields, contains pointers to their static
// description, provides public methods for accessing them by index and name.
// Field indexes are 0-based. Field names are case-sensitive.
class TRecordReflection : public TNonCopyable {
private:
    const TStaticRecordDescriptor& RecordDescriptor;
    THolder<TNameToIndexMap> NameToIndex;

public:
    TRecordReflection(const TStaticRecordDescriptor&);
    ~TRecordReflection();


    inline const char* GetRecordName() const noexcept {
        return RecordDescriptor.Name;
    }

    inline size_t GetRecordVersion() const noexcept {
        return RecordDescriptor.Version;
    }

    // Returns size of a record in bytes. Always a multiple of 4.
    inline size_t GetRecordSize() const noexcept {
        return RecordDescriptor.Size;
    }

    inline size_t GetFieldCount() const noexcept {
        return RecordDescriptor.FieldCount;
    }

    inline const TStaticFieldDescriptor& GetFieldDescriptor(size_t index) const noexcept {
        Y_ASSERT(index < GetFieldCount());
        return RecordDescriptor.Fields[index];
    }

    inline const TFieldReflection GetFieldReflection(size_t index) const noexcept {
        Y_ASSERT(index < GetFieldCount());
        return TFieldReflection(RecordDescriptor.Fields[index]);
    }

    // Field metadata methods

    inline const char* GetFieldName(size_t index) const noexcept {
        return GetFieldDescriptor(index).Name;
    }

    inline const char* GetFieldType(size_t index) const noexcept {
        return GetFieldDescriptor(index).Type;
    }

    inline bool IsBitField(size_t index) const noexcept {
        return GetFieldReflection(index).IsBitField();
    }

    inline ui32 GetFieldWidth(size_t index) const noexcept {
        return GetFieldDescriptor(index).Width;
    }

    inline ui32 GetFieldOffset(size_t index) const noexcept {
        return GetFieldDescriptor(index).Offset;
    }

    // Returns index of a field with given name or -1 if this field can not be found.
    // Accepts names listed in the PrevNames field of the codegen input.
    int GetFieldIndex(const TStringBuf& name) const noexcept;


    // Methods for manipulating bitfields in records

    inline ui32 GetBitField(const void* src, ui32 index) const noexcept {
        return GetFieldReflection(index).GetBitField(src);
    }

    inline void SetBitField(void* dst, ui32 index, ui32 value) const noexcept {
        GetFieldReflection(index).SetBitField(dst, value);
    }

    inline void CopyBitField(void* dst, const void* src, ui32 index) const noexcept {
        GetFieldReflection(index).CopyBitField(dst, src);
    }


    // Methods for manipulating C++ fields in records

    inline ui32 GetCppFieldSize(ui32 index) const noexcept {
        return GetFieldReflection(index).GetCppFieldSize();
    }


    inline const void* GetCppFieldPtr(const void* src, ui32 index) const noexcept {
        return GetFieldReflection(index).GetCppFieldPtr(src);
    }

    inline void* GetCppFieldPtr(void* src, ui32 index) const noexcept {
        return GetFieldReflection(index).GetCppFieldPtr(src);
    }

    inline void CopyCppField(void* dst, const void* src, ui32 index) const noexcept {
        GetFieldReflection(index).CopyCppField(dst, src);
    }

    template<typename T>
    inline const T& GetCppField(const void* src, ui32 index) const noexcept {
        return GetFieldReflection(index).GetCppField<T>(src);
    }

    template<typename T>
    inline void SetCppField(void* dst, ui32 index, const T& value) const noexcept {
        GetFieldReflection(index).SetCppField(dst, value);
    }


    // Generic field methods

    inline void ClearField(void* dst, ui32 index) const noexcept {
        GetFieldReflection(index).ClearField(dst);
    }

    inline void CopyField(void* dst, const void* src, ui32 index) const noexcept {
        GetFieldReflection(index).CopyField(dst, src);
    }

    template<typename T>
    inline T GetFieldUniversal(const void* src, ui32 index) const noexcept {
        return GetFieldReflection(index).GetFieldUniversal<T>(src);
    }

    template<typename T>
    inline void SetFieldUniversal(void* dst, ui32 index, const T& value) const noexcept {
        GetFieldReflection(index).SetFieldUniversal(dst, value);
    }
};

// Copies a field between records of different structures: dst[dstI] = src[srcI].
// Field must have the same type and width in both structures.
static inline void TranslateField(const TRecordReflection& dstR, void* dst, ui32 dstI, const TRecordReflection& srcR, const void* src, ui32 srcI) {
    Y_ASSERT(srcR.IsBitField(srcI) == dstR.IsBitField(dstI));
    Y_ASSERT(srcR.GetFieldWidth(srcI) == dstR.GetFieldWidth(dstI));

    if (Y_LIKELY(srcR.IsBitField(srcI)))
        dstR.SetBitField(dst, dstI, srcR.GetBitField(src, srcI));
    else
        memcpy(dstR.GetCppFieldPtr(dst, dstI), srcR.GetCppFieldPtr(src, srcI), srcR.GetCppFieldSize(srcI));
}

// Matches fields in two structures by their names.
// Returns a vector v in which v[i] is the index of a field in the second
// structure which corresponds to the i-th field in the first structure,
// or -1 if the i-th field was not matched with any other field.
// Checks that matched fields have the same type and size, and that no field
// can be matched to more than one another field, throws an exception if
// check fails.
TVector<int> MatchFieldsByName(const TRecordReflection& fields1, const TRecordReflection& fields2);

// A class for transforming records in batches as follows:
// for each dst field index i, set dst[i] = src[dst2src[i]] if dst2src[i] >= 0.
//
// If a dst2src vector is omitted, uses the result of MatchFieldsByName(dst, src).
//
// If source and destination records differ only slightly, a naive loop calling
// TranslateField() for each field may be up to about 20 times slower that memcpy.
// This class is intended to speed up this loop by detecting large blocks of
// bytes that are copied together and using memcpy for them.
//
// Each pair or corresponding source and destination fields must have
// the same type and size. Constructor verifies this and throws an exception
// if this is not the case. Reflection objects passed to the constructor must
// exists for the whole life of this object.
class TRecordTranslator {
    const TRecordReflection& DstReflection;
    const TRecordReflection& SrcReflection;

    struct TMove {
        ui16 Dst, Src;   // field indexes (if BlockSize=0) or byte offsets (for blocks)
        ui32 BlockSize;  // non-zero for block moves - block's size in bytes
    };
    TVector<TMove> Moves;

    void Init(const TVector<int>& dst2src);

public:
    TRecordTranslator(const TRecordReflection& dst, const TRecordReflection& src, const TVector<int>& dst2src);
    TRecordTranslator(const TRecordReflection& dst, const TRecordReflection& src);
    void Translate(void* dst, const void* src) const noexcept;
};


template<class TStruct>
void VisitFields(const TStruct& obj, typename TStruct::IFieldsVisitor& visitor);

template<class TStruct>
void VisitFields(const TStruct& obj, typename TStruct::IFieldsVisitor& visitor, const typename TStruct::TFieldMask& mask);


// For each field XXX in TStruct, contains a static method const TStaticFieldDescriptor XXX();
template<class TStruct>
struct TStaticReflection;

// Contains a static method NErf::TRecordReflection* Reflection();
template<class TStruct>
class TDynamicReflection;

} // namespace NErf
