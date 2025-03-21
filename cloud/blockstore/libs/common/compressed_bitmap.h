#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/memory/pool.h>

namespace NCloud::NBlockStore {

////////////////////////////////////////////////////////////////////////////////

class TCompressedBitmap
{
public:
    static const ui16 CHUNK_SIZE = 1024;

    struct TSerializedChunk
    {
        ui32 ChunkIdx;
        TStringBuf Data;
    };

public:
    TCompressedBitmap(size_t size);

    ~TCompressedBitmap();
    TCompressedBitmap(TCompressedBitmap&& other);
    TCompressedBitmap& operator=(TCompressedBitmap&& other);

public:
    void Clear();

private:
    void Init();

private:
    size_t Size;

    class TImpl;
    THolder<TImpl> Impl;

public:
    class TRangeSerializer
    {
    private:
        const TCompressedBitmap::TImpl* Parent = nullptr;
        ui64 First = 0;
        ui64 Last = 0;
        char* Buffer = nullptr;
        ui32 BufferPos = 0;

    public:
        TRangeSerializer() = default;

        TRangeSerializer(
            const TCompressedBitmap::TImpl& parent,
            ui64 first,
            ui64 last);

        ~TRangeSerializer();

    public:
        bool Next(TSerializedChunk* sc);
    };

    TRangeSerializer RangeSerializer(ui64 b, ui64 e) const;

    static bool IsZeroChunk(const TSerializedChunk& chunk);

    ui64 Set(ui64 b, ui64 e);
    ui64 Unset(ui64 b, ui64 e);
    void Update(const TSerializedChunk& chunk);
    ui64 Update(const TCompressedBitmap& other, ui64 b);
    ui64 Merge(const TSerializedChunk& chunk);
    ui64 Count() const;
    ui64 Count(ui64 b, ui64 e) const;
    bool Test(ui64 i) const;
    ui64 Capacity() const;
    ui64 MemSize() const;
};

}   // namespace NCloud::NBlockStore
