#pragma once

#include <util/system/filemap.h>
#include <util/stream/file.h>
#include <util/stream/mem.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/generic/maybe.h>
#include <util/folder/path.h>
#include <util/string/printf.h>
#include <util/memory/blob.h>
#include <util/system/sem.h>

class TArrayWithHeadBase {
public:
    static const size_t N_HEAD_SIZE = 1024;
};

template <class TArrayEl>
class TArrayWithHead: public TArrayWithHeadBase {
private:
    struct TBlobStorage {
        TBlob Blob;
        bool IsWritable = false;
        ui32 RecordSize = 0;
        size_t Size = 0;

        TBlobStorage(TBlob blob, bool isWritable, ui32 recordSize, size_t size)
            : Blob(std::move(blob))
            , IsWritable(isWritable)
            , RecordSize(recordSize)
            , Size(size)
        {}

        const TArrayEl& operator[](size_t docHandle) const {
            Y_ASSERT(docHandle < Size);
            return *(const TArrayEl*)(Blob.AsUnsignedCharPtr() + TArrayWithHeadBase::N_HEAD_SIZE + RecordSize * docHandle);
        }

        TArrayEl& operator[](size_t docHandle) {
            Y_ASSERT(docHandle < Size);
            Y_ASSERT(IsWritable);
            return *(TArrayEl*)(Blob.AsUnsignedCharPtr() + TArrayWithHeadBase::N_HEAD_SIZE + RecordSize * docHandle);
        }
    };

    ui32 RecordSize = 0;
    ui32 Version = 0;
    ui32 Time = 0;
    size_t Size = 0;
    TBlob GenericData;
    TMaybe<TVector<TArrayEl>> VectorStorage;
    TMaybe<TBlobStorage> BlobStorage;

    static TBlob MakeBlob(const TMemoryMap& map, bool quiet, IOutputStream& warnings, bool lockMemory) {
        if (lockMemory) {
            try {
                return TBlob::LockedFromMemoryMap(map, 0, map.Length());
            } catch (...) {
                if (!quiet) {
                    warnings << "Can't lock file: " << CurrentExceptionMessage() << "\n";
                }
                return TBlob::FromMemoryMap(map, 0, map.Length());
            }
        } else {
            return TBlob::FromMemoryMap(map, 0, map.Length());
        }
    }

    void InitStorage(TBlob blob, bool isWritable, const TString& filename, bool isPolite, bool quiet, IOutputStream& warnings);

public:
    typedef TArrayEl value_type;

    TArrayWithHead() = default;

    void Load(const TString& filename, bool isPolite = false, bool precharge = false, bool quiet = false, IOutputStream& warnings = Cerr, bool lockMemory = false);
    void Load(const TMemoryMap& mapping, bool isPolite = false, bool precharge = false, bool quiet = false, IOutputStream& warnings = Cerr, bool lockMemory = false);

    bool Load(const TString& filename, void* ptr, size_t len, bool isPolite, bool quiet, IOutputStream& warnings = Cerr);

    const TArrayEl& GetAt(size_t docHandle) const;

    size_t GetSize() const {
        return Size;
    }

    bool GetIsFile() const {
        return BlobStorage.Defined();
    }

    ui32 GetRecordSize() const {
        return RecordSize;
    }

    ui32 GetVersion() const {
        return Version;
    }

    ui32 GetTime() const {
        return Time;
    }

    const TBlob& GetGenericData() const {
        return GenericData;
    }

    void Resize(size_t newSize) {
        Y_ASSERT(!BlobStorage);
        auto& vector = VectorStorage ? *VectorStorage : VectorStorage.ConstructInPlace();
        vector.resize(newSize);
        if (newSize > Size)
            memset(&vector[Size], 0, (newSize - Size) * sizeof(TArrayEl));
        Size = newSize;
    }

    TArrayEl& operator[](size_t docHandle) noexcept {
        if (Y_LIKELY(BlobStorage)) {
            return (*BlobStorage)[docHandle];
        } else if (VectorStorage) {
            return (*VectorStorage)[docHandle];
        }
        Y_FAIL();
    }
};

void ArrayWithHeadInit(IOutputStream* out, ui32 size, ui32 version, ui32 time = 0, const TBlob& genericData = TBlob());

template <class TArrayEl>
class TArrayWithHeadWriter {
public:
    TArrayWithHeadWriter(IOutputStream* out, ui32 time = 0, const TBlob& genericData = TBlob())
        : Out(out)
    {
        ArrayWithHeadInit(out, sizeof(TArrayEl), TArrayEl::Version, time, genericData);
    }
    void Put(const TArrayEl& el) {
        Out->Write(&el, sizeof(TArrayEl));
    }
    void Put(const TArrayEl* el, size_t n) {
        Out->Write(el, n * sizeof(TArrayEl));
    }

private:
    IOutputStream* Out;
};

template <>
class TArrayWithHeadWriter<TString> {
public:
    TArrayWithHeadWriter(IOutputStream* out, ui32 time, const TBlob& genericData, size_t elemSize, ui32 version)
        : Out(out)
        , ElemSize(elemSize)
    {
        ArrayWithHeadInit(out, ElemSize, version, time, genericData);
    }
    void Put(const TString& el) {
        Y_VERIFY(el.size() == ElemSize, "Incorrect element size");
        Out->Write(el.data(), ElemSize);
    }

private:
    IOutputStream* Out;
    const size_t ElemSize;
};

template <class TArrayEl>
const TArrayEl& TArrayWithHead<TArrayEl>::GetAt(size_t docHandle) const {
    if (Y_LIKELY(BlobStorage)) {
        return (*BlobStorage)[docHandle];
    } else if (VectorStorage) {
        return (*VectorStorage)[docHandle];
    }
    Y_FAIL();
}

template <class TArrayEl>
class TPoliteArrayWithHead: public TArrayWithHead<TArrayEl> {
public:
    bool TryLoad(const char* name, bool isPolite = false, bool precharge = false) {
        try {
            TArrayWithHead<TArrayEl> temp;
            temp.Load(name, isPolite, precharge);
        } catch (const yexception&) {
            return false;
        }
        TArrayWithHead<TArrayEl>::Load(name, isPolite, precharge);
        return true;
    }

    static void Out(const char* info) {
        Y_UNUSED(info);
#if 0
       printf("%s\n", info);
#endif
    }

    struct TSemHolder {
        TSimpleSharedPtr<TSemaphore> Semaphore;
        TSemHolder(const char* name)
            : Semaphore(new TSemaphore(name, 1))
        {
            Semaphore->Acquire();
            Out("Acquired...");
        }
        ~TSemHolder() {
            Semaphore->Release();
            Out("Released...");
        }
    };

    void Load(const char* name, bool isPolite = false, bool precharge = false, IOutputStream& warnings = Cerr, bool lockMemory = false) {
        TArrayWithHead<TArrayEl>::Load(name, isPolite, precharge, false, warnings, lockMemory);
    }

    bool Load(const char* name, void* ptr, size_t len, bool isPolite, bool quiet, IOutputStream& warnings = Cerr) {
        return TArrayWithHead<TArrayEl>::Load(name, ptr, len, isPolite, quiet, warnings);
    }
};

void GetArrayHead(IInputStream* in, ui32* size, ui32* version, ui32* time = nullptr, TBlob* genericData = nullptr);
ui64 GetHeadArRecordNum(const TString& filename);

template <class TArrayEl>
void TArrayWithHead<TArrayEl>::Load(const TString& filename, bool isPolite, bool precharge, bool quiet, IOutputStream& warnings, bool lockMemory) {
    TMemoryMap map(filename, TFileMap::oRdOnly | (precharge ? TFileMap::oPrecharge : TFileMap::EOpenMode()));
    InitStorage(MakeBlob(map, quiet, warnings, lockMemory), false /*isWritable*/, filename, isPolite, quiet, warnings);
}

template <class TArrayEl>
void TArrayWithHead<TArrayEl>::Load(const TMemoryMap& mapping, bool isPolite, bool precharge, bool quiet, IOutputStream& warnings, bool lockMemory) {
    if (precharge) {
        TFileMap fileMap(mapping);
        fileMap.Map(0, static_cast<size_t>(fileMap.Length()));
        fileMap.Precharge();
    }
    InitStorage(MakeBlob(mapping, quiet, warnings, lockMemory), mapping.IsWritable(), mapping.GetFile().GetName(), isPolite, quiet, warnings);
}

template <class TArrayEl>
bool TArrayWithHead<TArrayEl>::Load(const TString& filename, void* data, size_t len, bool isPolite, bool quiet, IOutputStream& warnings) {
    InitStorage(TBlob::NoCopy(data, len), false /*isWritable*/, filename, isPolite, quiet, warnings);
    return !BlobStorage.Defined();
}

template <class TArrayEl>
void TArrayWithHead<TArrayEl>::InitStorage(TBlob blob, bool isWritable, const TString& filename, bool isPolite, bool quiet, IOutputStream& warnings) {
    BlobStorage.Clear();
    VectorStorage.Clear();
    size_t arrayHeadSize = TArrayWithHeadBase::N_HEAD_SIZE;
    const TString& qName = filename.Quote();

    const void* data = blob.Data();
    size_t len = blob.Length();
    Y_ENSURE(len >= arrayHeadSize,
             "Cannot read headed array from file " << qName << ". Head size should be at least " << arrayHeadSize << " bytes. ");

    TMemoryInput headerInput(data, arrayHeadSize);
    GetArrayHead(&headerInput, &RecordSize, &Version, &Time, &GenericData);

    size_t dataSize = len - arrayHeadSize;
    Y_ENSURE(RecordSize, "Cannot read headed array from file " << qName << ". Record count must be greater than zero. ");
    Y_ENSURE(0 == dataSize % RecordSize,
             "Cannot read headed array from file " << qName << ". Bad array size (fractional record count): space " << dataSize << " should be divisible by " << RecordSize << ". ");

    Size = dataSize / RecordSize;

    if (RecordSize < sizeof(TArrayEl)) {
        if (!isPolite)
            ythrow yexception() << "Can't read file " << qName << ":"
                                << " on-disk record size " << RecordSize
                                << " is less than in-memory record size " << ui32(sizeof(TArrayEl))
                                << " and polite mode is disabled. Read more: https://nda.ya.ru/3QTTeo";

        if (!quiet) {
            warnings << "File " << qName << " has on-disk record size " << RecordSize << ", in-memory record size is " << sizeof(TArrayEl) << "."
                                                                                                                                              " Requested polite mode will waste "
                     << ((Size * sizeof(TArrayEl) + 1048576 / 2) >> 20) << " Mb (for " << Size << " records). Read more: https://nda.ya.ru/3QTTeo\n";
        }

        VectorStorage.ConstructInPlace();
        Resize(Size);

        const unsigned char* ptr = static_cast<const unsigned char*>(data) + arrayHeadSize;
        for (size_t i = 0; i < Size; i++) {
            memcpy(&(*VectorStorage)[i], ptr, RecordSize);
            ptr += RecordSize;
        }
    } else {
        BlobStorage.ConstructInPlace(
            std::move(blob),
            isWritable,
            RecordSize,
            Size
        );
    }
}
