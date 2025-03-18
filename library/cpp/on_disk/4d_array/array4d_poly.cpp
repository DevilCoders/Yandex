#include "array4d_poly.h"

#include <library/cpp/on_disk/4d_array/array4d_poly_footer.pb.h>
#include <library/cpp/bit_io/bitoutput.h>

#include <util/folder/dirut.h>
#include <util/generic/algorithm.h>
#include <util/generic/buffer.h>
#include <util/generic/bitops.h>
#include <util/generic/map.h>
#include <util/generic/xrange.h>
#include <util/random/random.h>
#include <util/stream/aligned.h>
#include <util/stream/buffer.h>
#include <util/stream/file.h>
#include <util/stream/format.h>
#include <util/string/cast.h>
#include <util/system/align.h>
#include <util/system/filemap.h>
#include <util/system/tempfile.h>
#include <util/system/unaligned_mem.h>
#include <util/system/mlock.h>

using namespace NArray4DPoly;

// Reader private
namespace {
    // Importnat consts
    const char ARRAY_4D_POLY_MAGIC_VALUE[] = "Array4DPoly\0\0\0\0";
    const size_t KEY_WIDTH_BITS_NUM = 5;

    struct THeader {
        char Magic[16];
    };
    static_assert(sizeof(THeader) == 16, "expect sizeof(THeader) == 16");

    Y_FORCE_INLINE static ui64 GetBitField(const char* start, size_t shift, size_t len) noexcept {
        if (!len) {
            return 0;
        }
        Y_VERIFY(len < 58); // 58 == (1 + (8 * 7) + 1), such field occupies 9 bytes in the worst case
        const size_t bitsToRead = len + (shift % 8);
        const size_t bytesToRead = (bitsToRead / 8) + !!(bitsToRead % 8);
        ui64 res = 0;
        // read as few bytes as possible to avoid a buffer overrun when reading fields near to the right border
        memcpy(&res, start + (shift / 8), bytesToRead);
        res >>= (shift % 8);
        res &= MaskLowerBits(len);
        return res;
    }

    Y_FORCE_INLINE static size_t CalcWidth(size_t maxValue) noexcept {
        return maxValue ? GetValueBitCount(maxValue) : 0;
    }

    struct TKeyAndOffset {
        size_t Key;
        size_t Offset; // Starting offset for data of this key
        size_t LayerJump;

        TKeyAndOffset(size_t key, size_t offset)
            : Key(key)
            , Offset(offset)
            , LayerJump(0)
        {
        }
    };

    class TOutputStreamWrapper {
    public:
        TOutputStreamWrapper(TAlignedOutput& output, const TString& tempDir)
            : Output(output)
            , StartOffset(Output.GetCurrentOffset())
            , FileBased(!tempDir.empty())
            , TempFile(FileBased ? new TTempFile(tempDir + "/.tmp4DArrayIndex-" + ToString(RandomNumber<ui64>())) : nullptr)
            , Buffer(FileBased ? nullptr : new TBuffer)
            , BufferOutput(
                  FileBased
                      ? new TFileOutput(TempFile->Name())
                      : (IOutputStream*)new TBufferOutput(*Buffer))
            , BufferInput(
                  FileBased
                      ? new TIFStream(TempFile->Name(), 1 << 16)
                      : (IInputStream*)new TBufferInput(*Buffer))
            , LastKeyAndOffset(/*key=*/0, /*offset=*/0)
        {
        }

        void Write(const char* buf, size_t len) {
            Output.Write(buf, len);
        }

        template <class TOutputWrapper>
        void Save(TOutputWrapper& /*output*/) {
        }

        size_t GetCurrentOffset() const noexcept {
            return Output.GetCurrentOffset() - StartOffset;
        }

        size_t GetOffsetWidth(size_t /*totalBits*/, size_t /*count*/) const noexcept {
            return CalcWidth(GetCurrentOffset());
        }

        // index methods
        size_t GetIndexSize() const {
            return Size - Index;
        }

        size_t GetLastKey() const {
            return LastKeyAndOffset.Key;
        }

        void SetLastLayerJump(size_t layerJump) {
            LastKeyAndOffset.LayerJump = layerJump;
        }

        void Clear() {
            // In fact don't need anything, because this obhect is destroyed right after call
        }

        TKeyAndOffset PopFront() {
            ++Index;
            if (NeedFlush) {
                BufferOutput->Write(&LastKeyAndOffset, sizeof(LastKeyAndOffset));
                BufferOutput->Flush();
                NeedFlush = false;
            }
            TKeyAndOffset retNew(/*key=*/0, /*offset=*/0);
            size_t readSize = BufferInput->Load(&retNew, sizeof(retNew));
            Y_VERIFY(readSize == sizeof(retNew));
            return retNew;
        }

        bool IsEnd() const {
            return Index >= Size;
        }

        void PushBack(const TKeyAndOffset& keyAndOffset) {
            if (NeedFlush) {
                BufferOutput->Write(&LastKeyAndOffset, sizeof(LastKeyAndOffset));
            } else {
                NeedFlush = true;
            }
            LastKeyAndOffset = keyAndOffset;
            ++Size;
        }

    private:
        TAlignedOutput& Output;
        const size_t StartOffset;

        const bool FileBased;

        THolder<TTempFile> TempFile;
        THolder<TBuffer> Buffer;

        THolder<IOutputStream> BufferOutput;
        THolder<IInputStream> BufferInput;

        TKeyAndOffset LastKeyAndOffset;
        bool NeedFlush = false;
        size_t Index = 0;
        size_t Size = 0;
    };

    class TBufferWrapper {
    public:
        void Write(const char* buf, size_t len) {
            Buffer.Append(buf, len);
        }

        void FillWithZero(size_t len) {
            size_t size = Buffer.size();
            Buffer.Reserve(size + len);
            std::char_traits<char>::assign(Buffer.data() + size, len, '\0');
            Buffer.Advance(len);
        }

        template <class TOutputWrapper>
        void Save(TOutputWrapper& output) {
            output.Write(Buffer.data(), Buffer.size());
            Buffer.Reset();
        }

        size_t GetCurrentOffset() const noexcept {
            return Buffer.size();
        }

        size_t GetOffsetWidth(size_t totalBits, size_t count) const noexcept {
            size_t oldOffset = 0;
            while (true) {
                size_t fixedTotalBits = totalBits + oldOffset * count;
                size_t totalSize = GetCurrentOffset() + (fixedTotalBits + 7) / 8;
                size_t offset = CalcWidth(totalSize);
                Y_VERIFY(offset >= oldOffset, "Internal bug!");
                if (offset == oldOffset) {
                    break;
                }
                oldOffset = offset;
            }
            return oldOffset;
        }

        // index methods
        size_t GetIndexSize() const {
            return KeyAndOffsets.size();
        }

        size_t GetLastKey() const {
            return KeyAndOffsets.back().Key;
        }

        void SetLastLayerJump(size_t layerJump) {
            KeyAndOffsets.back().LayerJump = layerJump;
        }

        void Clear() {
            KeyAndOffsets.clear();
            Index = 0;
        }

        TKeyAndOffset PopFront() {
            return KeyAndOffsets[Index++];
        }

        bool IsEnd() const {
            return Index >= KeyAndOffsets.size();
        }

        void PushBack(const TKeyAndOffset& keyAndOffset) {
            KeyAndOffsets.push_back(keyAndOffset);
        }

    private:
        TBuffer Buffer;
        TVector<TKeyAndOffset> KeyAndOffsets;
        size_t Index = 0;
    };

    template <class TOutputWrapper>
    struct TBitOutputWrapperImpl {
        TBitOutputWrapperImpl(TOutputWrapper& output)
            : Output(output)
        {
        }

        void WriteData(const char* begin, const char* end) {
            Output.Write(begin, end - begin);
        }

        TOutputWrapper& Output;
    };

    template <class TOutputWrapper>
    class TBitOutputWrapper: public TBitOutputWrapperImpl<TOutputWrapper>, public NBitIO::TBitOutputBase<TBitOutputWrapperImpl<TOutputWrapper>> {
    public:
        TBitOutputWrapper(TOutputWrapper& output)
            : TBitOutputWrapperImpl<TOutputWrapper>(output)
            , NBitIO::TBitOutputBase<TBitOutputWrapperImpl<TOutputWrapper>>(this)
        {
        }
    };

    typedef TMap<size_t, size_t> TTypeSizeInfo; // itemKey -> min size
    typedef TArray4DPoly::TTypeInfo TTypeInfo;

    class TSimpleInputDataWrapper {
    public:
        TSimpleInputDataWrapper(const TArray4DPoly::TData& data, TTypeSizeInfo* typeSizeInfo)
            : Data(data)
            , TypeSizeInfo(typeSizeInfo)
        {
        }

        void Save(size_t itemKey, TBufferWrapper& output) {
            Save(itemKey, Data.Length, output);
        }

    protected:
        const TArray4DPoly::TData& Data;

        void Save(size_t itemKey, size_t realLength, TBufferWrapper& output) {
            if (TypeSizeInfo) {
                TTypeSizeInfo::iterator it = TypeSizeInfo->find(itemKey);
                if (it == TypeSizeInfo->end()) {
                    (*TypeSizeInfo)[itemKey] = realLength;
                } else if (it->second > realLength) {
                    it->second = realLength;
                }
            }
            output.Write(Data.Start, Data.Length);
        }

    private:
        TTypeSizeInfo* TypeSizeInfo;
    };

    class TResizerInputDataWrapper: protected  TSimpleInputDataWrapper {
    public:
        TResizerInputDataWrapper(const TArray4DPoly::TData& data, TTypeSizeInfo* typeSizeInfo, const TTypeInfo& typeResizer)
            : TSimpleInputDataWrapper(data, typeSizeInfo)
            , TypeResizer(typeResizer)
        {
        }

        void Save(size_t itemKey, TBufferWrapper& output) {
            // Calculate proper size
            size_t length = Data.Length;
            TTypeInfo::const_iterator it = TypeResizer.find(itemKey);
            ui32 newLength = (it == TypeResizer.end() || it->second < length) ? length : it->second;
            TSimpleInputDataWrapper::Save(itemKey, newLength, output);
            if (newLength > length) {
                output.FillWithZero(newLength - length);
            }
        }

    private:
        const TTypeInfo& TypeResizer;
    };

    template <class TDataWrapper, class TLayerTraits>
    class TLayerInfo {
    public:
        TLayerInfo() = default;

        template <class T1, class T2>
        TLayerInfo(T1& arg1, T2& arg2)
            : Data(arg1, arg2)
        {
        }

        template <class TInputDataWrapper>
        void Add(size_t* keyArray, TInputDataWrapper& data) {
            bool empty = Data.GetIndexSize() == 0;
            size_t newKey = keyArray[0];
            Y_VERIFY(
                empty || newKey >= Data.GetLastKey(),
                "Wrong new key: layerName=\"%s\" currentKey=%" PRISZT " newKey=%" PRISZT,
                TLayerTraits::LAYER_NAME,
                Data.GetLastKey(),
                newKey);
            if (!empty && (TLayerTraits::AllowDuplicates || newKey > Data.GetLastKey())) {
                Data.SetLastLayerJump(SubLayer.Flush(Data)); // Flush current
            }
            if (!TLayerTraits::HasKey) {
                size_t firstKey = empty ? 0 : Data.GetLastKey() + 1;
                for (size_t i = firstKey; i < newKey; ++i) {
                    AddKey(i);
                    SubLayer.Flush(Data);
                }
            }
            if (TLayerTraits::AllowDuplicates || empty || newKey > Data.GetLastKey()) {
                AddKey(newKey);
            }
            SubLayer.Add(keyArray + 1, data);
        }
        template <class TOutputWrapper>
        size_t Flush(TOutputWrapper& output) {
            size_t count = Data.GetIndexSize();
            if (count == 0) {
                return 0; // In fact return value doesn't matter
            } else if (!std::is_same<TLayerTraits, TRowsLayerTraits>::value && count == 1 && Data.GetLastKey() == /*defaultKey=*/0) {
                size_t ret = SubLayer.Flush(output);
                Data.Clear();
                return ret + 1;
            }
            Data.SetLastLayerJump(SubLayer.Flush(Data)); // Flush last

            // Write index:
            TBitOutputWrapper<TOutputWrapper> bitOutput(output);
            //     Write header
            static_assert(TLayerTraits::LayerId > 0, "expect TLayerTraits::LayerId > 0");
            size_t layerJumpWidth = CalcWidth(TLayerTraits::LayerId - 1);
            size_t totalBits = layerJumpWidth * count + TLayerTraits::ExtraCountBits;
            ui64 keyWidth = 0;
            if (TLayerTraits::HasKey) {
                keyWidth = CalcWidth(Data.GetLastKey());
                bitOutput.Write(keyWidth, KEY_WIDTH_BITS_NUM);
                totalBits += KEY_WIDTH_BITS_NUM + keyWidth * count;
            }
            size_t offsetWidth = Data.GetOffsetWidth(totalBits, count);

            // Writing count instead of first (zero) offset
            bitOutput.Write(count, offsetWidth + TLayerTraits::ExtraCountBits);
            TKeyAndOffset keyAndOffset = Data.PopFront();
            while (true) {
                if (TLayerTraits::HasKey) {
                    bitOutput.Write(keyAndOffset.Key, keyWidth);
                }
                bitOutput.Write(keyAndOffset.LayerJump, layerJumpWidth);
                if (Data.IsEnd()) {
                    break;
                }
                keyAndOffset = Data.PopFront();
                bitOutput.Write(keyAndOffset.Offset, offsetWidth);
            }
            bitOutput.Flush();

            // Write data (non-empty only for TBufferWrapper)
            Data.Save(output);

            Data.Clear();
            return 0;
        }

    private:
        TLayerInfo<TBufferWrapper, typename TLayerTraits::TSubLayer> SubLayer;

        TDataWrapper Data;

        void AddKey(size_t key) {
            TKeyAndOffset keyAndOffset(key, Data.GetCurrentOffset());
            Data.PushBack(keyAndOffset);
        }
    };
    template <>
    class TLayerInfo<TBufferWrapper, TTerminatingLayerTraits> {
    public:
        template <class TInputDataWrapper>
        void Add(size_t* keyArray, TInputDataWrapper& data) {
            data.Save(/*itemKey=*/keyArray[-1], Buffer); // Previous (item) layer has resizing key
        }
        template <class TOutputWrapper>
        size_t Flush(TOutputWrapper& output) {
            Buffer.Save(output);
            return 0;
        }

    private:
        TBufferWrapper Buffer;
    };

}

template <>
const char* TItemsLayerTraits::LAYER_NAME = "TItemsLayerTraits";
template <>
const char* TEntriesLayerTraits::LAYER_NAME = "TEntriesLayerTraits";
template <>
const char* TElementsLayerTraits::LAYER_NAME = "TElementsLayerTraits";
template <>
const char* TRowsLayerTraits::LAYER_NAME = "TRowsLayerTraits";

template <class TLayerTraits>
void TArray4DPoly::TLayerBase::Init(ui64 firstBits) {
    MetaShift = TLayerTraits::HasKey ? KEY_WIDTH_BITS_NUM : (size_t)TLayerTraits::ExtraCountBits;
    KeyWidth = TLayerTraits::HasKey ? firstBits & ((0x1 << KEY_WIDTH_BITS_NUM) - 1) : 0;
    LayerJumpWidth = TLayerTraits::LayerJumpWidth;
    Count = (TLayerTraits::HasKey ? firstBits >> KEY_WIDTH_BITS_NUM : firstBits) & MaskLowerBits(OffsetWidth + TLayerTraits::ExtraCountBits);
}

TArray4DPoly::TLayerBase::TLayerBase(
    const char* index,
    TData data) noexcept
    : Index(index)
    , OffsetWidth(CalcWidth(data.Length))
    , DataStart(data.Start)
    , DataLength(data.Length)
{
    ui64 firstBits = data.Length ? ReadUnaligned<ui64>(index) : 0;
    Init<TRowsLayerTraits>(firstBits);
}

template <class TLayerTraits>
void TArray4DPoly::TLayerBase::Init(const char* dataStart, size_t dataLength, size_t layerJump) noexcept {
    SkipLayers = layerJump;
    if (TLayerTraits::LayerId - 1 - TTerminatingLayerTraits::LayerId == layerJump) {
        // Most of these fields are not used
        Index = nullptr;
        MetaShift = 0;
        OffsetWidth = 0;
        KeyWidth = 0;
        LayerJumpWidth = 0;
        Count = (bool)dataLength;
        DataStart = dataStart;
        DataLength = dataLength;
    } else {
        Index = dataStart;
        OffsetWidth = CalcWidth(dataLength);
        ui64 firstBits = dataLength ? ReadUnaligned<ui64>(dataStart) : 0;
        if ((size_t)TLayerTraits::LayerId == (size_t)TEntriesLayerTraits::LayerId || TLayerTraits::LayerId - 1 - TItemsLayerTraits::LayerId == layerJump) {
            Init<TItemsLayerTraits>(firstBits);
        } else if ((size_t)TLayerTraits::LayerId == (size_t)TElementsLayerTraits::LayerId || (size_t)TLayerTraits::LayerId - 1 - TEntriesLayerTraits::LayerId == layerJump) {
            Init<TEntriesLayerTraits>(firstBits);
        } else {
            Y_ASSERT((size_t)TLayerTraits::LayerId - 1 - TElementsLayerTraits::LayerId == layerJump);
            Init<TElementsLayerTraits>(firstBits);
        }
        size_t indexSize = (MetaShift + Count * (OffsetWidth + KeyWidth + LayerJumpWidth) + 7) / 8;
        DataStart = dataStart + indexSize;
        DataLength = dataLength - indexSize;
    }
}

template <class TLayerTraits>
TArray4DPoly::TLayerBase::TLayerBase(const TBuffer& buf, size_t layerJump, const TLayerTraits& /*constructorSelector*/) noexcept {
    Init<TLayerTraits>(buf.Begin(), buf.Size(), layerJump);
}

template <class TLayerTraits>
TArray4DPoly::TLayerBase::TLayerBase(const TLayer<TLayerTraits>& supLayer, size_t pos) noexcept {
    const TLayerBase& base = supLayer;
    if ((size_t)TLayerTraits::LayerId != (size_t)TRowsLayerTraits::LayerId && base.SkipLayers) { // rows do not have skips
        Index = base.Index;
        MetaShift = base.MetaShift;
        OffsetWidth = base.OffsetWidth;
        KeyWidth = base.KeyWidth;
        LayerJumpWidth = base.LayerJumpWidth;
        Count = base.Count;
        SkipLayers = base.SkipLayers - 1;
        DataStart = base.DataStart;
        DataLength = base.DataLength;
    } else {
        size_t step = base.OffsetWidth + base.KeyWidth + base.LayerJumpWidth;
        size_t shift1 = base.MetaShift + pos * step;
        size_t shift2 = shift1 + step;
        size_t shiftJ = shift2 - base.LayerJumpWidth;
        ui64 expectedOffset1 = GetBitField(base.Index, shift1, base.OffsetWidth);
        ui64 expectedOffset2 = GetBitField(base.Index, shift2, base.OffsetWidth);
        size_t layerJump = GetBitField(base.Index, shiftJ, base.LayerJumpWidth);
        size_t offset1 = pos ? expectedOffset1 : 0;
        size_t offset2 = (pos + 1 == base.Count) ? base.DataLength : expectedOffset2;
        const char* dataStart = base.DataStart + offset1;
        size_t dataLength = offset2 - offset1;
        Init<TLayerTraits>(dataStart, dataLength, layerJump);
    }
}

template <>
TArray4DPoly::TLayer<TElementsLayerTraits>::TLayer(const TBuffer& buf, size_t layerJump) noexcept
    : TLayerBase(buf, layerJump, TRowsLayerTraits())
{
}

size_t TArray4DPoly::TLayerBase::GetKey(size_t pos) const noexcept {
    return GetBitField(Index, MetaShift + OffsetWidth + pos * (OffsetWidth + KeyWidth + LayerJumpWidth), KeyWidth);
}
bool TArray4DPoly::TLayerBase::TryFindPosByKey(const size_t beginPos, const size_t endPos, const size_t key, size_t& pos) const noexcept {
    const auto shift = MetaShift + OffsetWidth;
    const auto width = OffsetWidth + KeyWidth + LayerJumpWidth;

    const auto getKeyByPos = [&](const size_t thePos) {
        return GetBitField(Index, shift + thePos * width, KeyWidth);
    };

    const auto range = xrange(beginPos, endPos);
    const auto iter = LowerBound(range.begin(), range.end(), key, [&](const size_t thePos, const size_t k) {
        return getKeyByPos(thePos) < k;
    });

    if (iter != range.end() && getKeyByPos(*iter) == key) {
        pos = *iter;
        return true;
    }

    return false;
}
TArray4DPoly::TData TArray4DPoly::TLayerBase::GetData(size_t pos) const noexcept {
    size_t step = OffsetWidth + KeyWidth + LayerJumpWidth;
    size_t shift1 = MetaShift + pos * step;
    size_t shift2 = shift1 + step;
    ui64 expectedOffset1 = GetBitField(Index, shift1, OffsetWidth);
    ui64 expectedOffset2 = GetBitField(Index, shift2, OffsetWidth);
    size_t offset1 = pos ? expectedOffset1 : 0;
    size_t offset2 = (pos + 1 == Count) ? DataLength : expectedOffset2;
    return TData(DataStart + offset1, offset2 - offset1);
}

// Safe defaults
TArray4DPoly::TArray4DPoly() {
}

TArray4DPoly::~TArray4DPoly() {
    if (LockDataMemory && !Data.IsNull()) {
        UnlockMemory(Data.Data(), Data.Size());
    }
}

void TArray4DPoly::Load(const TMemoryMap& mapping, const TTypeInfo& requiredTypes, bool isPolite, bool quiet, bool memLock) {
    Load(TBlob::FromMemoryMap(mapping, 0, mapping.Length()), mapping.GetFile().GetName(), requiredTypes, isPolite, quiet, memLock);
}

void TArray4DPoly::Load(const TBlob& blob, const TString& debugName, const TTypeInfo& requiredTypes, bool isPolite, bool quiet, bool memLock) {
    Y_ENSURE(Data.IsNull(), "Load() has already been called");
    Data = blob;
    DoLoad(debugName.Quote(), requiredTypes, isPolite, quiet, memLock);
}

void TArray4DPoly::DoLoad(const TString& fileName, const TTypeInfo& requiredTypes, bool isPolite, bool quiet, bool memLock) {
    const size_t fileSize = Data.Size();
    const char* const data = Data.AsCharPtr();
    if (LockDataMemory = memLock) {
        LockMemory(Data.Data(), Data.Size());
    }

    // Read static header with magic
    size_t requiredSize = sizeof(THeader);
    Y_ENSURE(fileSize >= requiredSize, fileName + " file is truncated");
    const THeader* header = (const THeader*)data;
    Y_ENSURE(memcmp(header->Magic, ARRAY_4D_POLY_MAGIC_VALUE, sizeof(header->Magic)) == 0, fileName + " is not a valid TArray4DPoly file");

    // Read proto footer size
    requiredSize += sizeof(ui64);
    Y_ENSURE(fileSize >= requiredSize, fileName + " file is truncated");
    const char* end = data + fileSize - sizeof(ui64);
    ui64 protoFooterSize = ReadUnaligned<ui64>(end);
    requiredSize += protoFooterSize;
    Y_ENSURE(fileSize >= requiredSize, fileName + " file is truncated");

    // Read proto footer
    TArray4DPolyFooter protoFooter;
    end -= protoFooterSize;
    bool footerParsed = protoFooter.ParseFromArray(end, protoFooterSize);
    Y_ENSURE(footerParsed, "Parsing of protobuf footer has failed");

    // Get index
    const char* index = (data + protoFooter.GetIndexOffset());
    Y_ENSURE(fileSize >= protoFooter.GetIndexOffset(), fileName + " file is corrupted");

    // Get rows data
    Y_ENSURE(protoFooter.GetIndexOffset() >= protoFooter.GetDataOffset(), " file is corrupted");
    TData rowsData(data + protoFooter.GetDataOffset(), protoFooter.GetIndexOffset() - protoFooter.GetDataOffset());

    TopLayer = TRowsLayer(rowsData, index);

    // Check for type sizes
    bool needPolitness = false;
    for (size_t i = 0; i < protoFooter.TypeInfoSize(); ++i) {
        const TArray4DPolyFooter::TTypeInfo& typeInfo = protoFooter.GetTypeInfo(i);
        size_t typeNumber = typeInfo.GetItemKey();
        size_t typeSize = typeInfo.GetMinSize();
        TTypeInfo::const_iterator it = requiredTypes.find(typeNumber);
        if (it != requiredTypes.end() && it->second > typeSize) {
            Y_ENSURE(isPolite,
                     "Can't read file " << fileName << ":"
                                        << " on-disk record size " << typeSize
                                        << " is less than in-memory record size " << it->second
                                        << " for type " << typeNumber
                                        << " and polite mode is disabled.");
            needPolitness = true;
            break;
        }
    }
    if (needPolitness) {
        TBuffer buffer;
        TBufferOutput output(buffer);
        {
            TArray4DPolyWriter writer(output);
            writer.SetResizer(requiredTypes);

            for (size_t rowId = 0; rowId < TopLayer.GetCount(); ++rowId) {
                TElementsLayer elementsLayer = TopLayer.GetSubLayer(rowId);
                for (size_t elementId = 0; elementId < elementsLayer.GetCount(); ++elementId) {
                    TEntriesLayer entriesLayer = elementsLayer.GetSubLayer(elementId);
                    for (size_t entryId = 0; entryId < entriesLayer.GetCount(); ++entryId) {
                        TItemsLayer itemsLayer = entriesLayer.GetSubLayer(entryId);
                        size_t entryKey = entriesLayer.GetKey(entryId);
                        for (size_t itemId = 0; itemId < itemsLayer.GetCount(); ++itemId) {
                            writer.Add(
                                /*rowId=*/rowId,
                                /*elementId=*/elementId,
                                /*entryKey=*/entryKey,
                                /*itemKey=*/itemsLayer.GetKey(itemId),
                                /*data=*/itemsLayer.GetData(itemId));
                        }
                    }
                }
            }
        }
        buffer.ShrinkToFit();
        if (!quiet) {
            Cerr << "[WARNING] Requested polite mode wasted " << HumanReadableSize(buffer.Capacity(), SF_BYTES) << " to load "
                                                                                                                   "comparing to "
                 << HumanReadableSize(fileSize, SF_BYTES) << " of the " << fileName << " file." << Endl;
        }
        Data = TBlob::FromBuffer(buffer);
        // This self call must succeed. So using isPolite=false!
        DoLoad(fileName + "(resized in memory for politness)", requiredTypes, /*isPolite=*/false, /*quiet=*/false, memLock);
    }
}

class TArray4DPolyWriter::TImpl: private TNonCopyable {
public:
    TImpl(IOutputStream& externalStream, TString tempDir)
        : RawOutput(externalStream)
        , Output(&RawOutput)
        , RowsInfo(Init(), tempDir)
    {
    }

    TImpl(const TString& name, TString tempDir)
        : OutputHolder(new TFileOutput(name))
        , RawOutput(*OutputHolder)
        , Output(&RawOutput)
        , RowsInfo(Init(), tempDir)
    {
    }

    ~TImpl() {
        Finalize();
    }

    void Add(size_t* keyArray, const TArray4DPoly::TData& data) {
        if (!data.Length) { // Fool proof
            return;
        }
        if (!TypeResizer) {
            TSimpleInputDataWrapper dataWrapper(data, &TypeSizeInfo);
            RowsInfo.Add(keyArray, dataWrapper);
        } else {
            TResizerInputDataWrapper resizerDataWrapper(data, &TypeSizeInfo, *TypeResizer);
            RowsInfo.Add(keyArray, resizerDataWrapper);
        }
    }

    void SetResizer(const TTypeInfo& requiredTypes) {
        TypeResizer.Reset(new TTypeInfo(requiredTypes));
    }

private:
    // Type info
    TTypeSizeInfo TypeSizeInfo;
    THolder<TTypeInfo> TypeResizer;

    // Meta index info
    TArray4DPolyFooter Footer;

    // Stream holding & wrapping
    THolder<IOutputStream> OutputHolder;
    IOutputStream& RawOutput;
    TAlignedOutput Output;

    TLayerInfo<TOutputStreamWrapper, TRowsLayerTraits> RowsInfo;

    TAlignedOutput& Init() {
        size_t headerSize = Y_ARRAY_SIZE(ARRAY_4D_POLY_MAGIC_VALUE);
        Output.Write(std::begin(ARRAY_4D_POLY_MAGIC_VALUE), headerSize);
        Footer.SetDataOffset(Output.GetCurrentOffset()); // Index offset
        return Output;
    }
    void Finalize() {
        RowsInfo.Flush(RawOutput); // Index size is not affecting current offset in Output
        // Index
        Footer.SetIndexOffset(Output.GetCurrentOffset()); // Index offset

        // Type info table
        for (TTypeSizeInfo::const_iterator it = TypeSizeInfo.begin(); it != TypeSizeInfo.end(); ++it) {
            TArray4DPolyFooter::TTypeInfo* typeInfo = Footer.AddTypeInfo();
            typeInfo->SetItemKey(it->first);
            typeInfo->SetMinSize(it->second);
        }

        // Footer
        size_t footerOffset = Output.GetCurrentOffset();
        bool goodSerialized = Footer.SerializeToArcadiaStream(&Output);
        Y_ENSURE(goodSerialized, "Writing TArray4DPoly has failed on serializing footer");
        size_t afterFooterOffset = Output.GetCurrentOffset();

        // Footer size
        ui64 footerSize = afterFooterOffset - footerOffset;
        Output.Write(&footerSize, sizeof(footerSize));
    }
};

TArray4DPolyWriter::TArray4DPolyWriter(IOutputStream& externalStream, TString tempDir)
    : Impl(new TImpl(externalStream, tempDir))
{
}

TArray4DPolyWriter::TArray4DPolyWriter(const TString& name, TString tempDir)
    : Impl(new TImpl(name, tempDir))
{
}

void TArray4DPolyWriter::Add(size_t rowId, size_t elementId, size_t entryKey, size_t itemKey, const TData& data) {
    size_t keyArray[4] = {rowId, elementId, entryKey, itemKey};
    Impl->Add(keyArray, data);
}
void TArray4DPolyWriter::SetResizer(const TTypeInfo& requiredTypes) {
    Impl->SetResizer(requiredTypes);
}

TArray4DPolyWriter::~TArray4DPolyWriter() {
}

class TArray4DPolyRowsWriter::TImpl {
private:
    TBufferOutput RawOutput;
    TTypeInfo TypeResizer;
    TLayerInfo<TBufferWrapper, TElementsLayerTraits> ElementsInfo;

public:
    TImpl(const TTypeInfo& requiredTypes)
        : TypeResizer(requiredTypes)
    {
    }

    void Add(size_t* keyArray, const TArray4DPoly::TData& data) {
        if (!data.Length) { // Fool proof
            return;
        }
        TResizerInputDataWrapper resizerDataWrapper(data, /*typeSizeInfo=*/nullptr, TypeResizer);
        ElementsInfo.Add(keyArray, resizerDataWrapper);
    }

    void Finalize(TBuffer& buf, char& layerJump) {
        layerJump = ElementsInfo.Flush(RawOutput); // Index size is not affecting current offset in Output
        buf = RawOutput.Buffer();
        RawOutput.Buffer().Clear();
    }
};

TArray4DPolyRowsWriter::TArray4DPolyRowsWriter(const TTypeInfo& requiredTypes)
    : Impl(new TImpl(requiredTypes))
{
}

TArray4DPolyRowsWriter::~TArray4DPolyRowsWriter() {
}

void TArray4DPolyRowsWriter::Add(size_t elementId, size_t entryKey, size_t itemKey, const TData& data) {
    size_t keyArray[3] = {elementId, entryKey, itemKey};
    Impl->Add(keyArray, data);
}

void TArray4DPolyRowsWriter::Finalize(TBuffer& buf, char& layerJump) {
    Impl->Finalize(buf, layerJump);
}

// Manual instantiation for incredibly smart compilers
template TArray4DPoly::TLayerBase::TLayerBase(const TLayer<TRowsLayerTraits>& supLayer, size_t pos) noexcept;
template TArray4DPoly::TLayerBase::TLayerBase(const TLayer<TElementsLayerTraits>& supLayer, size_t pos) noexcept;
template TArray4DPoly::TLayerBase::TLayerBase(const TLayer<TEntriesLayerTraits>& supLayer, size_t pos) noexcept;

template TArray4DPoly::TLayerBase::TLayerBase(const TBuffer& buf, size_t layerJump, const TRowsLayerTraits& constructorSelector) noexcept;
