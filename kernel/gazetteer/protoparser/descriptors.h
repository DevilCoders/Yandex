#pragma once

#include "protoparser.h"

#include <kernel/gazetteer/common/serialize.h>

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor_database.h>

#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>

namespace NGzt {


typedef NProtoBuf::Descriptor TDescriptor;
typedef NProtoBuf::DescriptorProto TDescriptorProto;
typedef NProtoBuf::FileDescriptor TFileDescriptor;
typedef NProtoBuf::FileDescriptorProto TFileDescriptorProto;
typedef NProtoBuf::FieldDescriptor TFieldDescriptor;


// forward declarations from binary.proto
namespace NBinaryFormat { class TProtoPool; }


class TFileDescriptorCollection {
public:
    TFileDescriptorCollection(TErrorCollector* errors = nullptr);
    ~TFileDescriptorCollection();

    bool AddFile(TFileDescriptorProto* file) {
        FileIndex[file->name()] = Files.size();
        Files.push_back(file);
        return Database.AddAndOwn(file);
    }

    // Add all @files
    bool AddCollection(const TFileDescriptorCollection& files);

    // Transform added "raw" proto descriptors into real file descriptors.
    bool Compile();

    void Save(NBinaryFormat::TProtoPool& proto) const;
    void Load(const NBinaryFormat::TProtoPool& proto);

    size_t Size() const {
        return Files.size();
    }

    const TFileDescriptorProto* operator[](size_t index) const {
        return Files[index];
    }

    bool HasFile(const TString& fileName) const {
        return FileIndex.find(fileName) != FileIndex.end();
    }

    const NProtoBuf::DescriptorPool& Pool() const {
        return Pool_;
    }

    // Return prototype of message either from generated_pool (preferred), or from DynamicMessageFactory
    const TMessage* SelectPrototype(const TDescriptor* generated, const TDescriptor* runtime) const;

private:
    TErrorCollector* Errors;
    NProtoBuf::SimpleDescriptorDatabase Database;
    NProtoBuf::DescriptorPool Pool_;

    TVector<TFileDescriptorProto*> Files;
    THashMap<TString, ui32> FileIndex;            // file name -> index in Files

    // Holds DynamicMessageFactory, is should always be deleted BEFORE corresponding DescriptorPool is deleted.
    class TPrototyper;
    THolder<TPrototyper> Prototyper;
};


enum ERelation {
    UNRELATED,
    COMMON_PARENT,
    PARENT_CHILD,
    CHILD_PARENT,
    SAME
};


class THierarchy {
public:
    const TVector<ui32>& Base() const {
        return BaseIndex;
    }

    void SetBase(size_t derived, size_t base) {
        GrowUpto(Max(derived, base) + 1);
        BaseIndex[derived] = base;
    }

    void GrowUpto(size_t n) {
        while (n > BaseIndex.size())
            BaseIndex.push_back(BaseIndex.size());
    }

    void Save(NBinaryFormat::TProtoPool& proto) const;
    void Load(const NBinaryFormat::TProtoPool& proto);


    ERelation GetRelation(size_t i1, size_t i2) const;

private:
    // Returns depth of descriptor (specified by @index) in a tree of descriptors = number of parent descriptors
    // Descriptor without parent (root) has depth = 0
    size_t GetParentCount(ui32 index) const;

    // Returns @n-th parent of descriptor specified by @index, parents are numbered starting from 1 to GetParentCount()
    // The first parent is immediate base type, its base is second parent and so on.
    ui32 GetNthParent(ui32 index, size_t n) const;

private:
    TVector<ui32> BaseIndex;              // i -> index of parent (base)
                                          // (i -> i denotes no base)
};




inline const NProtoBuf::DescriptorPool* GeneratedPool() {
    return NProtoBuf::DescriptorPool::generated_pool();
}

inline bool FromGeneratedPool(const TDescriptor* descriptor) {
    return descriptor->file()->pool() == GeneratedPool();
}


class TDescriptorIndex {
public:
    size_t Size() const {
        return Descriptors.size();
    }

    // Descriptor by its index
    const TDescriptor* GetDescriptor(size_t index) const {
        return Descriptors[index].Best;
    }

    const TDescriptor* operator[](size_t index) const {
        return GetDescriptor(index);
    }

    // Reverse: an index by descriptor pointer, use for serialization mainly
    ui32 GetIndex(const TDescriptor* type) const {
        THashMap<const TDescriptor*, ui32>::const_iterator index = Index.find(type);
        Y_ASSERT(index != Index.end());
        return index->second;
    }

    bool GetIndex(const TDescriptor* type, ui32& index) const {
        THashMap<const TDescriptor*, ui32>::const_iterator it = Index.find(type);
        if (it == Index.end())
            return false;
        index = it->second;
        return true;
    }

    // DescriptorPool-like method
    const TDescriptor* FindMessageTypeByName(const TString& name) const {
        return Pool != nullptr ? SelectBest(Pool->FindMessageTypeByName(name)) : nullptr;
    }


    TUtf16String GetDescriptorName(const TDescriptor* type) const {
        ui32 index = 0;
        return GetIndex(type, index) ? Descriptors[index].Name : TUtf16String();
    }

    TUtf16String GetDescriptorFullName(const TDescriptor* type) const {
        ui32 index = 0;
        return GetIndex(type, index) ? Descriptors[index].FullName : TUtf16String();
    }


    // True, if @checked_type represents same message-type as compiled-in @builtin_type.
    bool IsSameAsGeneratedType(const TDescriptor* checked, const TDescriptor* generated) const {
        Y_ASSERT(FromGeneratedPool(generated));
        return SelectGenerated(checked) == generated;
    }

    bool IsSameAsGeneratedTypeField(const TFieldDescriptor* checked_field, const TDescriptor* generated_type) const {
        return checked_field->cpp_type() == TFieldDescriptor::CPPTYPE_MESSAGE &&
               IsSameAsGeneratedType(checked_field->message_type(), generated_type);
    }

    // Return first field-descriptor of type TSearchKey for specified @descriptor
    const TFieldDescriptor* GetSearchKeyField(const TDescriptor* descriptor) const;

protected:
    bool Reset(const NProtoBuf::DescriptorPool& pool, const TVector<TString>& fullNames,
               TErrorCollector* errors = nullptr);

    const TDescriptor* SelectBest(const TDescriptor* d) const {
        ui32 index = 0;
        return GetIndex(d, index) ? Descriptors[index].Best : d;
    }

    const TDescriptor* SelectGenerated(const TDescriptor* d) const {
        if (d == nullptr || FromGeneratedPool(d))
            return d;
        ui32 index = 0;
        return GetIndex(d, index) ? Descriptors[index].Generated : nullptr;
    }

private:
    void Add(const TDescriptor* descr);

    void Clear() {
        Descriptors.clear();
        Index.clear();
    }

private:
    struct TInfo {
        const TDescriptor* Runtime;         // Dynamically built from Pool
        const TDescriptor* Generated;       // Identical from generated_pool
        const TDescriptor* Best;            // Used one: Generated if not NULL, otherwise Runtime
        const TFieldDescriptor* SearchKey;  // Descriptor of first search-key field.
        TUtf16String Name, FullName;              // Decoded name/full_name, for mixing with article titles

        inline TInfo(const TDescriptor* runtime);
    };

    TVector<TInfo> Descriptors;
    THashMap<const TDescriptor*, ui32> Index;

    const NProtoBuf::DescriptorPool* Pool{};
};



class TDescriptorCollection: public TDescriptorIndex {
public:
    TDescriptorCollection(TErrorCollector* errors = nullptr)
        : Files(errors)
    {
    }

    void Load(const NBinaryFormat::TProtoPool& proto);

    // Index of base type, if no base type, @index returned
    size_t GetBaseIndex(size_t index) const {
        return Hierarchy.Base()[index];
    }

    const TDescriptor* GetBase(size_t index) const {
        size_t base = GetBaseIndex(index);
        return base == index ? nullptr : GetDescriptor(base);
    }

    // pointer to base-type descriptor if any, or NULL
    const TDescriptor* GetBase(const TDescriptor* type) const {
        ui32 index = 0;
        return GetIndex(type, index) ? GetBase(index) : nullptr;
    }

    ERelation GetRelation(const TDescriptor* first, const TDescriptor* second) const {
        ui32 i1, i2;
        return GetIndex(first, i1) && GetIndex(second, i2) ? Hierarchy.GetRelation(i1, i2) : UNRELATED;
    }

    const TMessage* GetPrototype(const TDescriptor* descriptor) const {
        return Files.SelectPrototype(SelectGenerated(descriptor), descriptor);
    }

protected:
    TFileDescriptorCollection Files;
    THierarchy Hierarchy;

    friend class TDescriptorCollectionBuilder;      // for accessing Files
};


class TDescriptorCollectionBuilder: public TDescriptorCollection {
public:
    TDescriptorCollectionBuilder(TErrorCollector* errors = nullptr)
        : TDescriptorCollection(errors)
        , Errors(errors)
    {
    }

    bool AddFile(TFileDescriptorProto* file) {
        return Files.AddFile(file);
    }

    // Register proto-descriptor in order of parsing.
    // Note that containing file proto-descriptors should be also added to collection using AddFile()
    bool AddMessage(TDescriptorProto* msgProto, const TString& fullName);

    bool ImportPool(const TDescriptorCollection& collection);

    // Transform added "raw" proto descriptors into real descriptors.
    bool Compile();



    bool HasFile(const TString& fileName) const {
        return Files.HasFile(fileName);
    }

    void ExportFiles(TSet<TString>& files) const {
        for (size_t i = 0; i < Files.Size(); ++i)
            files.insert(Files[i]->name());
    }

    TDescriptorProto* FindMessageProto(const TString& fullName) const {
         THashMap<TString, ui32>::const_iterator it = MessageIndex.find(fullName);
         return it != MessageIndex.end() ? Messages[it->second] : nullptr;
    }

    const NProtoBuf::DescriptorPool& Pool() const {
        return Files.Pool();
    }

    void Save(NBinaryFormat::TProtoPool& proto) const;

private:
    void AddError(const TString& message) {
        if (Errors)
            Errors->AddError(message);
    }

private:
    TErrorCollector* Errors;

    TVector<TDescriptorProto*> Messages;   // raw descriptor proto (owned by Pool)
    TVector<TString> FullNames;             // descriptor full names
    THashMap<TString, ui32> MessageIndex;      // descriptor full name -> index at Messages
};





}   // namespace NGzt
