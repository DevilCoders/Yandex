#pragma once

#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/generic/set.h>
#include <util/generic/map.h>
#include <util/generic/string.h>
#include <util/stream/mem.h>
#include <util/stream/buffer.h>
#include <util/stream/length.h>
#include <util/memory/blob.h>
#include <util/ysaveload.h>
#include <util/stream/file.h>
#include <util/stream/buffer.h>
#include <util/system/tempfile.h>
#include <util/random/random.h>

#include <google/protobuf/message.h>
#include <google/protobuf/messagext.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/coded_stream.h>

#include "tools.h"

namespace NGzt
{

// Defined in this file:
class TMemoryInputStreamAdaptor;
class TBlobCollection;


typedef NProtoBuf::Message TMessage;


// allows transferring data from TMemoryInput to NProtoBuf::io::ZeroCopyInputStream without intermediate bufferring.
class TMemoryInputStreamAdaptor: public NProtoBuf::io::ZeroCopyInputStream {
public:
    TMemoryInputStreamAdaptor(TMemoryInput* input)
        : Input(input)
        , LastData(nullptr)
        , LastSize(0)
        , BackupSize(0)
        , TotalRead(0)
    {
    }

    ~TMemoryInputStreamAdaptor() override {
        // return unread memory back to Input
        if (BackupSize > 0)
            Input->Reset(LastData + (LastSize - BackupSize), BackupSize + Input->Avail());
    }

    // implements ZeroCopyInputStream ----------------------------------
    bool Next(const void** data, int* size) override {
        if (Y_UNLIKELY(BackupSize > 0)) {
            Y_ASSERT(LastData != nullptr && LastSize >= BackupSize);
            size_t rsize = CutSize(BackupSize);
            *data = LastData + (LastSize - BackupSize);
            *size = rsize;
            TotalRead += rsize;
            BackupSize -= rsize;
            return true;

        } else if ((LastSize = Input->Next(&LastData)) != 0) {
            size_t rsize = CutSize(LastSize);
            *data = LastData;
            *size = rsize;
            TotalRead += rsize;
            BackupSize = LastSize - rsize;          // in case when LastSize > Max<int>() do auto-backup
            return true;

        } else {
            LastData = nullptr;
            LastSize = 0;
            return false;
        }
    }

    void BackUp(int count) override {
        Y_ASSERT(count >= 0 && BackupSize + count <= LastSize);
        BackupSize += count;
        TotalRead -= count;
    }

    bool Skip(int count) override {
        int skipped = 0, size = 0;
        const void* data = nullptr;
        while (skipped < count && Next(&data, &size))
            skipped += size;
        if (skipped < count)
            return false;
        if (skipped > count)
            BackUp(skipped - count);
        return true;
    }

    int64_t ByteCount() const override {
        return TotalRead;
    }

private:
    size_t CutSize(size_t sz) const {
        return Min<size_t>(sz, Max<int>());
    }

private:
    TMemoryInput* Input;
    const char* LastData;
    size_t LastSize, BackupSize;
    int64_t TotalRead;
};

} // namespace NGzt


inline void SaveProtectedSize(IOutputStream* output, size_t size) {
    if (Y_UNLIKELY(static_cast<ui64>(size) > static_cast<ui64>(Max<ui32>())))
        ythrow yexception() << "Size " << size << " exceeds ui32 limits.";
    ::Save<ui32>(output, size);
    ::Save<ui32>(output, IntHash<ui32>(size));
}

inline bool LoadProtectedSize(IInputStream* input, size_t& res) {
    ui32 size = 0, hash = 0;
    ::Load<ui32>(input, size);
    ::Load<ui32>(input, hash);
    if (hash == IntHash<ui32>(size)) {
        res = size;
        return true;
    }
    return false;
}

inline size_t LoadProtectedSize(IInputStream* input) {
    size_t size;
    if (!LoadProtectedSize(input, size)) {
        TString msg = "Failed to load data: incompatible format or corrupted binary.";
        Cerr << msg << Endl;
        ythrow yexception() << msg;
    }
    return size;
}

// containers protection: to prevent allocating random (if corrupted) big amount of memory
template <typename T>
inline void SaveProtected(IOutputStream* output, const T& data) {
    ::SaveProtectedSize(output, data.size());       // just prepend extra ui64 (instead of redefining ::Save/::Load for all containers)
    ::Save(output, data);
}

template <typename T>
inline void LoadProtected(IInputStream* input, T& data) {
    ::LoadProtectedSize(input);    // fail if a size seems to be corrupted
    ::Load(input, data);
}


namespace NGzt {

class TMessageSerializer: public NProtoBuf::io::TProtoSerializer {
    typedef NProtoBuf::io::TProtoSerializer TBase;
    typedef NProtoBuf::Message TMessage;
public:
    using TBase::Save;
    using TBase::Load;

    // a little bit faster then with IInputStream
    static inline void Load(TMemoryInput* input, TMessage& msg) {
        NGzt::TMemoryInputStreamAdaptor adaptor(input);
        NProtoBuf::io::CodedInputStream decoder(&adaptor);
        if (!TBase::Load(&decoder, msg))
            ythrow yexception() << "Cannot read protobuf Message from input stream";
    }

    // override TBase::Load
    static inline void Load(IInputStream* input, TMessage& msg) {
        if (dynamic_cast<TMemoryInput*>(input) != nullptr)
            Load(static_cast<TMemoryInput*>(input), msg);
        else
            TBase::Load(input, msg);
    }

    // skip message without parsing.
    static inline bool Skip(NProtoBuf::io::CodedInputStream* input) {
        ui32 size;
        return input->ReadVarint32(&size) && input->Skip(size);
    }
};






// Collection of binary large objects which are serialized separately from other gazetteer data.
//
// When gazetteer is serialized it transforms its inner structures to special protobuf objects first
// (described in binary.proto) and then uses protobuf serialization to save them in binary format.
// But not all gazetteer data serialized in this way. There are several big objects (BLOBs)
// which are already serialized and which are later on de-serialization just mapped into memory, without copying.
// These, for example, are TArticlePool::ArticleData (all articles as protobuf binary)
// and all inner TCompactTrie of TGztTrie as well.
// All such blobs are stored separately in TBlobCollection and referred from other data
// by corresponding blob key.
class TBlobCollection : TNonCopyable
{
public:
    class TError: public yexception {};

    inline TBlobCollection() {
    }

    inline void Save(IOutputStream* output) const {
        ::SaveProtectedSize(output, Blobs.size());
        for (TMap<TString, TBlob>::const_iterator it = Blobs.begin(); it != Blobs.end(); ++it) {
            ::Save(output, it->first);
            SaveBlob(output, it->second);
        }
    }

    inline void Load(TMemoryInput* input) {
        size_t count = ::LoadProtectedSize(input);
        for (size_t i = 0; i < count; ++i) {
            TString key;
            ::Load(input, key);
            Add(key, LoadBlob(input));
        }
    }

    const TBlob& operator[] (const TString& blobkey) const {
        TMap<TString, TBlob>::const_iterator res = Blobs.find(blobkey);
        if (res == Blobs.end())
            ythrow TError() << "No blob with key \"" << blobkey << "\" found in collection.";
        else
            return res->second;
    }

    inline bool HasBlob(const TString& blobkey) const {
        return Blobs.find(blobkey) != Blobs.end();
    }

    void SaveRef(const TString& key, const TBlob& blob, TString* blobkey) {
        Add(key, blob);
        blobkey->assign(key);
    }

    inline void SaveNoCopy(const TString& key, TBuffer& buffer, TString* blobkey) {
        // @buffer is cleared after operation
        SaveRef(key, TBlob::FromBuffer(buffer), blobkey);
    }

    inline void SaveCopy(const TString& key, const TBuffer& buffer, TString* blobkey) {
        SaveRef(key, TBlob::Copy(buffer.Data(), buffer.Size()), blobkey);
    }

    template <typename T>
    inline void SaveCompactTrie(const TString& key, const T& trie, TString* blobkey) {
        TBufferOutput tmpbuf;
        trie.Save(tmpbuf);
        SaveNoCopy(key, tmpbuf.Buffer(), blobkey);
    }

    template <typename T>
    inline void SaveObject(const TString& key, const T& object, TString* blobkey) {
        // any arcadia-serializeable object (with ::Save defined)
        TBufferOutput tmpbuf;
        ::Save(&tmpbuf, object);
        SaveNoCopy(key, tmpbuf.Buffer(), blobkey);
    }

    template <typename T>
    inline void LoadObject(T& object, const TString& blobkey) const {
        // any arcadia-serializeable object (with ::Load defined)
        const TBlob& blob = (*this)[blobkey];
        TMemoryInput input(blob.Data(), blob.Length());
        ::Load(&input, object);
    }


private:
    static inline void SaveBlob(IOutputStream* output, const TBlob& blob) {
        Y_ASSERT(blob.Size() <= Max<ui32>());
        ::SaveProtectedSize(output, blob.Size());
        ::SaveArray(output, blob.AsCharPtr(), blob.Size());
    }

    static inline TBlob LoadBlob(TMemoryInput* input) {
        size_t blob_len = 0;

        if (::LoadProtectedSize(input, blob_len)) {
            const char* data;
            size_t data_len = input->Next(&data);
            if (data_len == 0 || data_len < blob_len)
                ythrow yexception() << "Unexpected end of memory stream.";
            input->Reset(data + blob_len, data_len - blob_len);
            return TBlob::NoCopy(data, blob_len);
        } else
            return TBlob();
    }

    inline void Add(const TString& key, const TBlob& blob) {
        if (Blobs.find(key) != Blobs.end())
            ythrow TError() << "Two blobs with same key \"" << key << "\". Keys must be unique.";
        Blobs[key] = blob;
    }

    TMap<TString, TBlob> Blobs;
};


// TSentinel is used for validation of serialized data.
// A sentinel (special fixed 8-byte sequence) is inserted between blocks of data on serialization
// and verified on de-serialization

class TSentinel
{
private:
    static inline ui64 SentinelValue()
    {
        return 0x0DF0EFBEADDECEFAULL; //1004566319143505658ULL;  //0xFACEDEADBEEFF00D reversed;
    }

public:
    static inline void Set(IOutputStream* output)
    {
        ::Save(output, SentinelValue());
    }

    static inline bool Check(IInputStream* input)
    {
        ui64 n = 0;
        ::Load(input, n);
        return n == SentinelValue();
    }

    class TMissingSentinelError: public yexception
    {
    };

    static inline void Verify(IInputStream* input)
    {
        if (!Check(input))
        {
            Cerr << "Failed to de-serialize data: incompatible format or corrupted binary." << Endl;
            ythrow TMissingSentinelError();
        }
    }
};

template <typename TValue>
struct TRepeatedFieldTypeSelector {
    typedef NProtoBuf::RepeatedField<TValue> TField;

    static inline void AddTo(TField* field, TValue value) {
        field->Add(value);
    }
};

template <>
struct TRepeatedFieldTypeSelector<TString> {
    typedef NProtoBuf::RepeatedPtrField<TString> TField;

    static inline void AddTo(TField* field, TString value) {
        *field->Add() = value;
    }
};


template <typename TItemIterator>
inline void SaveToField(typename TRepeatedFieldTypeSelector<typename TItemIterator::TValue>::TField* field,
                        const TItemIterator& itemIter, size_t itemSize)
{
    field->Reserve(field->size() + itemSize);
    for (; itemIter.Ok(); ++itemIter)
        TRepeatedFieldTypeSelector<typename TItemIterator::TValue>::AddTo(field, *itemIter);
}

template <typename TItemCollection>
inline void SaveToField(typename TRepeatedFieldTypeSelector<typename TItemCollection::value_type>::TField* field,
                        const TItemCollection& items)
{
    field->Reserve(field->size() + items.size());
    for (typename TItemCollection::const_iterator it = items.begin(); it != items.end(); ++it)
        TRepeatedFieldTypeSelector<typename TItemCollection::value_type>::AddTo(field, *it);
}

template <typename TRepeatedField, typename TVectorType>
inline void LoadVectorFromField(const TRepeatedField& field, TVectorType& items)
{
    items.reserve(field.size());
    for (int i = 0; i < field.size(); ++i)
        items.push_back(field.Get(i));
}

template <typename TRepeatedField, typename TSetType>
inline void LoadSetFromField(const TRepeatedField& field, TSetType& items)
{
    for (int i = 0; i < field.size(); ++i)
        items.insert(field.Get(i));
}


template <typename TProtoObject, typename TObject>
inline void SaveAsProto(IOutputStream* output, const TObject& object)
{
    TProtoObject proto;
    TBlobCollection blobs;
    object.Save(proto, blobs);

    TMessageSerializer::Save(output, proto);
    TSentinel::Set(output);

    // blobs
    blobs.Save(output);
    TSentinel::Set(output);
}

template <typename TProtoObject, typename TObject>
inline void LoadAsProto(TMemoryInput* input, TObject& object)
{
    TProtoObject proto;
    TBlobCollection blobs;
    LoadProtoBlobs(input, proto, blobs);
    object.Load(proto, blobs);
}

template <typename TProtoObject>
inline void LoadProtoOnly(TMemoryInput* input, TProtoObject& proto)
{
    TMessageSerializer::Load(input, proto);
    TSentinel::Verify(input);
}

template <typename TProtoObject>
inline void LoadProtoBlobs(TMemoryInput* input, TProtoObject& proto, TBlobCollection& blobs)
{
    LoadProtoOnly(input, proto);
    blobs.Load(input);
    TSentinel::Verify(input);
}



class TTempFileOutput: public IOutputStream {
public:
    TTempFileOutput(const TString& fileName)
        : FileName(fileName)
        , TmpFile(fileName + "." + ToString(RandomNumber<ui64>()))
        , Slave(TmpFile.Name())
    {
    }

    void Commit() {
        Finish();
        if (!NFs::Rename(TmpFile.Name(), FileName))
            ythrow yexception() << "cannot rename temp file";
    }

protected:
    void DoWrite(const void* buf, size_t len) override {
        Slave.Write(buf, len);
    }

    void DoFlush() override {
        Slave.Flush();
    }

    void DoFinish() override {
        Slave.Finish();
    }

private:
    TString FileName;
    TTempFile TmpFile;
    TOFStream Slave;
};


} // namespace NGzt
