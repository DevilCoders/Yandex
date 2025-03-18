#pragma once

#include <util/generic/hash.h>
#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/memory/blob.h>

// 4d polymorph array:
//     rows -> elements -> entries -> items
//
// File structure:
//     Static header - magic to identify format
//     Rows data - concatenated blobs of all rows
//     Rows index - normal layer index, but after data
//     Protobuf footer - makes format more flexible and backward compatible
//     Footer size - ui64
//
// Rows/elements/entries layers of the same structure:
//     Offsets + Key + LayerJump - offsets of elements x number of elements
//                             (first offset is replaced with count of elements and
//                                extended by fixed layer dependent amount of bits)
//     Elements/entries/items - variable width elements/entries/items
//
// Keys are sorted inside one element, doesn't have duplicates
// Types are sorted inside one entry, do have duplicates
// LayerJump determines amount of layers to skip
//

class TBuffer;
class TFileMap;
class TMemoryMap;

template <bool hasKey, class TSubLayerTraits, size_t extraCountBits = 0>
class TLayerTraits {
public:
    enum {
        // Layer id
        LayerId = TSubLayerTraits::LayerId + 1,
        // Has key
        HasKey = hasKey,
        // Duplicate keys policy
        AllowDuplicates = LayerId == 1,
        // Extra bits
        ExtraCountBits = extraCountBits,
        // Layer jump
        LayerJumpWidth = TSubLayerTraits::LayerJumpWidth + (bool)((1u << TSubLayerTraits::LayerJumpWidth) == TSubLayerTraits::LayerId)
    };
    typedef TSubLayerTraits TSubLayer;

    static const char* LAYER_NAME;
};

class TTerminatingLayerTraits {
public:
    enum {
        LayerId = 0,
        LayerJumpWidth = 0
    };
};
typedef TLayerTraits</*hasKey*/ true, TTerminatingLayerTraits> TItemsLayerTraits;
typedef TLayerTraits</*hasKey*/ true, TItemsLayerTraits> TEntriesLayerTraits;
typedef TLayerTraits</*hasKey*/ false, TEntriesLayerTraits, 1> TElementsLayerTraits;
typedef TLayerTraits</*hasKey*/ false, TElementsLayerTraits, 26> TRowsLayerTraits;

class TArray4DPoly: private TNonCopyable {
public:
    struct TData {
        const char* Start;
        size_t Length;

        TData(const char* start = nullptr, size_t length = 0) noexcept
            : Start(start)
            , Length(length)
        {
        }
    };
    typedef THashMap<size_t, size_t> TTypeInfo; // TypeNumber -> TypeSize

    template <class TLayerTraits>
    class TLayer;

    class TLayerBase {
    public:
        size_t GetCount() const noexcept {
            return SkipLayers ? (bool)Count : Count;
        }

    protected:
        TLayerBase() noexcept {
            // Empty *current* layer (no skips)
        }
        TLayerBase(
            const char* index,
            TData data) noexcept;

        template <class TLayerTraits>
        TLayerBase(const TLayer<TLayerTraits>& supLayer, size_t pos) noexcept;

        template <class TLayerTraits>
        TLayerBase(const TBuffer& buf, size_t layerJump, const TLayerTraits& constructorSelector) noexcept;

        size_t GetKey(size_t pos) const noexcept;
        bool TryFindPosByKey(size_t beginPos, size_t endPos, size_t key, size_t& pos) const noexcept; // applies lower bound algorithm

        TData GetData(size_t pos) const noexcept;
        bool NeedSkip() const noexcept {
            return SkipLayers;
        }
        TData GetData() const noexcept {
            return TData(DataStart, DataLength);
        }

    private:
        const char* Index = nullptr;
        size_t MetaShift = 0;
        size_t OffsetWidth = 0;
        size_t KeyWidth = 0;
        size_t LayerJumpWidth = 0;
        size_t Count = 0;
        size_t SkipLayers = 0; // Amount of layers to skip
        const char* DataStart = nullptr;
        size_t DataLength = 0;

        template <class TLayerTraits>
        inline void Init(ui64 firstBits);

        template <class TLayerTraits>
        inline void Init(const char* dataStart, size_t dataLength, size_t layerJump) noexcept;
    };

    template <class TLayerTraits>
    class TLayer: public TLayerBase {
    public:
        TLayer() noexcept {
            // default empty layer
        }
        TLayer(const TData& data, const char* index) noexcept
            : TLayerBase(
                  /*index=*/index,
                  /*data=*/data) {
        }
        TLayer(const TBuffer& buf, size_t layerJump) noexcept;
        TLayer<typename TLayerTraits::TSubLayer> GetSubLayer(size_t pos) const noexcept {
            Y_ASSERT(pos < TLayerBase::GetCount());
            static_assert((size_t)TLayerTraits::LayerId != (size_t)TItemsLayerTraits::LayerId, "expect (size_t)TLayerTraits::LayerId != (size_t)TItemsLayerTraits::LayerId");
            typedef TLayer<typename TLayerTraits::TSubLayer> TSubLayer;
            return TSubLayer(*this, pos);
        }
        size_t GetKey(size_t pos) const noexcept {
            Y_ASSERT(pos < TLayerBase::GetCount());
            static_assert(TLayerTraits::HasKey, "expect TLayerTraits::HasKey");
            if (NeedSkip()) {
                return 0; // Default key for skipped layers
            }
            return TLayerBase::GetKey(pos);
        }
        bool TryFindPosByKey(const size_t beginPos, const size_t endPos, const size_t key, size_t& pos) const noexcept {
            Y_ASSERT(endPos <= TLayerBase::GetCount());
            if (NeedSkip()) {
                if (key == 0) {
                    pos = 0;
                    return true;
                }
                return false;
            }
            return TLayerBase::TryFindPosByKey(beginPos, endPos, key, pos);
        }
        TData GetData(size_t pos) const noexcept {
            Y_ASSERT(pos < TLayerBase::GetCount());
            static_assert(TLayerTraits::LayerId == TItemsLayerTraits::LayerId, "expect TLayerTraits::LayerId == TItemsLayerTraits::LayerId");
            if (NeedSkip()) {
                return TLayerBase::GetData();
            }
            return TLayerBase::GetData(pos);
        }

    private:
        template <class TOtherLayerTraits>
        TLayer(const TLayer<TOtherLayerTraits>& super, size_t pos) noexcept
            : TLayerBase(super, pos)
        {
            typedef std::is_same<typename TOtherLayerTraits::TSubLayer, TLayerTraits> TSameTypeTraits;
            static_assert(TSameTypeTraits::value, "expect TSameTypeTraits::value");
        }

        template <class TOtherLayerTraits>
        friend class TLayer;
    };
    typedef TLayer<TItemsLayerTraits> TItemsLayer;
    typedef TLayer<TEntriesLayerTraits> TEntriesLayer;
    typedef TLayer<TElementsLayerTraits> TElementsLayer;
    typedef TLayer<TRowsLayerTraits> TRowsLayer;

    size_t GetCount() const noexcept {
        return TopLayer.GetCount();
    }
    TElementsLayer GetSubLayer(size_t pos) const noexcept {
        return TopLayer.GetSubLayer(pos);
    }

    TArray4DPoly();
    ~TArray4DPoly();

    void Load(const TMemoryMap& mapping, const TTypeInfo& requiredTypes, bool isPolite = false, bool quiet = false, bool memLock = false);
    void Load(const TString& fileName, const TTypeInfo& requiredTypes, bool isPolite = false, bool quiet = false, bool memLock = false) {
        auto blob = memLock ? TBlob::PrechargedFromFile(fileName) : TBlob::FromFile(fileName);
        Load(std::move(blob), fileName, requiredTypes, isPolite, quiet, memLock);
    }
    void Load(const TBlob& blob, const TString& debugName, const TTypeInfo& requiredTypes, bool isPolite = false, bool quiet = false, bool memLock = false);

private:
    // Internal data:
    //     Holding data
    TBlob Data;
    bool LockDataMemory = false;

    TRowsLayer TopLayer;

    // Internal methods:
    //     Load implementation
    void DoLoad(const TString& fileName, const TTypeInfo& requiredTypes, bool isPolite, bool quiet, bool memLock);
};

class IArray4DPolyWriter {
public:
    typedef TArray4DPoly::TData TData;

public:
    virtual void Add(size_t rowId, size_t elementId, size_t entryKey, size_t itemKey, const TData&) = 0;
    virtual ~IArray4DPolyWriter() {
    }
};

class TArray4DPolyWriter: public IArray4DPolyWriter, private TNonCopyable {
public:
    // Holds top index data in memory in case of empty tempDir
    TArray4DPolyWriter(IOutputStream& externalStream, TString tempDir = TString());
    TArray4DPolyWriter(const TString& name, TString tempDir = TString());
    ~TArray4DPolyWriter() override;

    void SetResizer(const TArray4DPoly::TTypeInfo& requiredTypes);
    void Add(size_t rowId, size_t elementId, size_t entryKey, size_t itemKey, const TData&) override;

private:
    class TImpl;
    THolder<TImpl> Impl;
};

class TArray4DPolyRowsWriter: private TNonCopyable {
public:
    typedef TArray4DPoly::TData TData;
    typedef TArray4DPoly::TTypeInfo TTypeInfo;

    TArray4DPolyRowsWriter(const TTypeInfo& requiredTypes);
    ~TArray4DPolyRowsWriter();

    void Add(size_t elementId, size_t entryKey, size_t itemKey, const TData&);

    void Finalize(TBuffer& buf, char& layerJump);

private:
    class TImpl;
    THolder<TImpl> Impl;
};
