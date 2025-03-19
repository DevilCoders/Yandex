#pragma once

#include "factor_view.h"
#include "float_utils.h"

#include <util/generic/array_ref.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/memory/pool.h>
#include <util/generic/array_ref.h>
#include <utility>

struct TGeneralResizeableFactorStorage;
class TFactorStorage;

using TFactorStoragePtr = TFactorStorage*;
using TConstFactorStoragePtr = const TFactorStorage*;

template <typename TStorage = TFactorStorage, typename TPool, typename... Args>
inline TStorage* CreateFactorStorageInPool(TPool* pool, Args&&... args) {
    Y_ASSERT(pool);
    return new (pool->Allocate(sizeof(TStorage))) TStorage(std::forward<Args>(args)..., pool);
}

// NOTE. Factor storage doesn't support
// contaners such as TVector<TStorage> with
// allocation in pool. This is intentional,
// aimed to prevent tricky misuse scenarios
// where storage remains alive after the pool
// was destroyed leading to memory access violations.
struct TGeneralResizeableFactorStorage {
private:
    class IStorageAlloc {
    public:
        virtual ~IStorageAlloc() = default;

        virtual void* Allocate(ui32 size) = 0;
        virtual void Release(void* ptr) = 0;
        virtual bool IsInPool(void* pool) = 0;

        template <typename T>
        T* Allocate() {
            return static_cast<T*>(Allocate(sizeof(T)));
        }
    };

    class TInHeapStorageAlloc : public IStorageAlloc {
    public:
        TInHeapStorageAlloc() = default;

        static TInHeapStorageAlloc* Instance() {
            return Singleton<TInHeapStorageAlloc>();
        }

        void* Allocate(ui32 size) final {
            return new char[size];
        }
        void Release(void* ptr) final {
            delete [] static_cast<char*>(ptr);
        }
        bool IsInPool(void* /*pool*/) final {
            return false;
        }
    };

    template <typename TPool>
    class TInPoolStorageAlloc
        : public IStorageAlloc
    {
    public:
        explicit TInPoolStorageAlloc(TPool* pool)
            : Pool(pool)
        {
            Y_ASSERT(Pool);
        }

        static TInPoolStorageAlloc<TPool>* Instance(TPool* pool) {
            Y_ASSERT(pool);
            return new (pool->Allocate(sizeof(TInPoolStorageAlloc<TPool>))) TInPoolStorageAlloc<TPool>(pool);
        }

        void* Allocate(ui32 size) final {
            return static_cast<char*>(Pool->Allocate(size));
        }
        void Release(void*) final {
        }
        bool IsInPool(void* pool) final {
            return static_cast<void*>(Pool) == pool;
        }

    private:
        TPool* Pool = nullptr;
    };

private:
    TGeneralResizeableFactorStorage(ui32 count, IStorageAlloc* alloc)
        : Alloc(alloc)
    {
        Y_ASSERT(Alloc);
        if (count > 0) {
            factors = static_cast<float*>(Alloc->Allocate(sizeof(float) * count));
            FactorsCount = count;
            FactorsCapacity = count;
        }
    }

public:
    float& operator[](size_t i) {
        Y_ASSERT(i < FactorsCount);
        return factors[i];
    }
    float operator[](size_t i) const {
        Y_ASSERT(i < FactorsCount);
        return factors[i];
    }

    static void FloatClear(float *dst, float *end) {
        NFactorViewPrivate::FloatClear(dst, end);
    }
    static void FloatMove(float* fromBegin, float* fromEnd, ptrdiff_t offset);

    void ClearFirstNFactors(size_t n) {
        FloatClear(factors, factors + n);
    }
    void Clear() {
        FloatClear(factors, factors + FactorsCount);
    }

    explicit TGeneralResizeableFactorStorage(ui32 count)
        : TGeneralResizeableFactorStorage(count, static_cast<IStorageAlloc*>(TInHeapStorageAlloc::Instance()))
    {
    }
    template<class TPool>
    TGeneralResizeableFactorStorage(ui32 count, TPool* pool)
        : TGeneralResizeableFactorStorage(count, static_cast<IStorageAlloc*>(TInPoolStorageAlloc<TPool>::Instance(pool)))
    {
    }
    ~TGeneralResizeableFactorStorage() {
        Alloc->Release(Domain);
        Alloc->Release(factors);
    }

    // NOTE. Default move-ctor reallocates in heap, if needed
    TGeneralResizeableFactorStorage(TGeneralResizeableFactorStorage&& other)
    {
        if (TInHeapStorageAlloc::Instance() == other.Alloc) {
            DoSwap(*this, other);
        } else {
            *this = other; // Copy if other is not in heap
        }
    }
    template <typename TPool>
    TGeneralResizeableFactorStorage(TGeneralResizeableFactorStorage&& other, TPool* pool)
        : Alloc(TInPoolStorageAlloc<TPool>::Instance(pool))
    {
        if (other.Alloc->IsInPool(pool)) {
            DoSwap(*this, other);
        } else {
            *this = other; // Copy if other is not in pool
        }
    }
    // NOTE. Default copy-ctor always allocates in heap.
    TGeneralResizeableFactorStorage(const TGeneralResizeableFactorStorage& other)
    {
        *this = other;
    }
    template <typename TPool>
    TGeneralResizeableFactorStorage(const TGeneralResizeableFactorStorage& other, TPool* pool)
        : Alloc(TInPoolStorageAlloc<TPool>::Instance(pool))
    {
        *this = other;
    }

    // NOTE. Assignment doesn't change storage allocation, e.g.
    // "storageInHeap = storageInPool" doesn't relocate storageInHeap
    // from heap to pool; only contents are copied.
    TGeneralResizeableFactorStorage& operator=(const TGeneralResizeableFactorStorage& other) {
        if (Y_LIKELY(&other != this)) {
            Y_ASSERT(!other.factors || factors != other.factors);
            Y_ASSERT(other.factors || 0 == other.FactorsCount);
            ResizeFactors<false>(other.FactorsCount);
            if (other.FactorsCount > 0) {
                memcpy(factors, other.factors, sizeof(float) * other.FactorsCount);
            }

            if (other.Domain) {
                *AllocateDomain() = *other.Domain;
            }
        }
        return *this;
    }

    TGeneralResizeableFactorStorage& operator=(TGeneralResizeableFactorStorage&& other) {
        if (TInHeapStorageAlloc::Instance() == other.Alloc) {
            DoSwap(*this, other);
        } else {
            *this = other; // Copy if other is not in heap
        }
        return *this;
    }

    void Swap(TGeneralResizeableFactorStorage& other) {
        DoSwap(factors, other.factors);
        DoSwap(FactorsCount, other.FactorsCount);
        DoSwap(FactorsCapacity, other.FactorsCapacity);
        DoSwap(Domain, other.Domain);
        DoSwap(Alloc, other.Alloc);
    }

    size_t Size() const {
        return FactorsCount;
    }
    void Resize(ui32 count) {
        ResizeFactors<true>(count);
    }

    void Save(IOutputStream* rh) const;
    void Save(IOutputStream* rh, size_t i) const;
    void Load(IInputStream* rh);

public:
    float* factors = nullptr;

private:
    friend class TFactorStorage;

    IStorageAlloc* Alloc = TInHeapStorageAlloc::Instance();
    ui32 FactorsCount = 0;
    ui32 FactorsCapacity = 0;
    TFactorDomain* Domain = nullptr;

    template <bool NeedCopy>
    void ResizeFactors(ui32 count) {
        if (count > FactorsCapacity) {
            auto newFactors = static_cast<float*>(Alloc->Allocate(sizeof(float) * count));
            if (NeedCopy && FactorsCount > 0) {
                memcpy(newFactors, factors, sizeof(float) * FactorsCount);
            }
            Alloc->Release(factors);
            factors = newFactors;
            FactorsCapacity = count;
        }
        FactorsCount = count;
        Y_ASSERT(FactorsCount <= FactorsCapacity);
    }
    TFactorDomain* AllocateDomain() {
        if (!Domain) {
            Domain = Alloc->Allocate<TFactorDomain>();
        }
        return Domain;
    }
};

TString SerializeFactorBorders(const TFactorStorage& storage);

class TBasicFactorStorage {
    friend bool ValidAndHasFactors(const TBasicFactorStorage* storage);

public:
    static EFactorUniverse Universe;

    float* factors = nullptr; // for backward compatibility, use Ptr() instead

public:
    TBasicFactorStorage(const TBasicFactorStorage& other)
        : Storage(other.Storage)
    {
        factors = Storage.factors;
    }
    template <typename TPool>
    TBasicFactorStorage(const TBasicFactorStorage& other, TPool* pool)
        : Storage(other.Storage, pool)
    {
        factors = Storage.factors;
    }

    TBasicFactorStorage(TBasicFactorStorage&& other)
        : Storage(std::move(other.Storage))
    {
        factors = Storage.factors;
        other.factors = nullptr;
    }
    template <typename TPool>
    TBasicFactorStorage(TBasicFactorStorage&& other, TPool* pool)
        : Storage(std::move(other.Storage), pool)
    {
        factors = Storage.factors;
        other.factors = nullptr;
    }

    explicit TBasicFactorStorage(ui32 count)
        : Storage(count)
    {
        factors = Storage.factors;
    }
    template <typename TPool>
    TBasicFactorStorage(ui32 count, TPool* pool)
        : Storage(count, pool)
    {
        factors = Storage.factors;
    }

    TBasicFactorStorage& operator=(const TBasicFactorStorage& other) {
        Storage = other.Storage;
        factors = Storage.factors;
        return *this;
    }
    TBasicFactorStorage& operator=(TBasicFactorStorage&& other) {
        Storage = std::move(other.Storage);
        factors = Storage.factors;
        return *this;
    }
    void Swap(TBasicFactorStorage& other) {
        DoSwap(factors, other.factors);
        DoSwap(Storage, other.Storage);
    }
    void Swap(TGeneralResizeableFactorStorage& other) {
        factors = other.factors;
        DoSwap(Storage, other);
    }

    void Save(IOutputStream* rh) const {
        Storage.Save(rh);
    }
    void Save(IOutputStream* rh, size_t i) const {
        Storage.Save(rh, i);
    }
    void Load(IInputStream* rh) {
        Storage.Load(rh);
    }

    void ClearFirstNFactors(size_t n) {
        Y_ASSERT(n <= Storage.Size());
        NFactorViewPrivate::FloatClear(Storage.factors, Storage.factors + n);
    }
    void Clear() {
        NFactorViewPrivate::FloatClear(Storage.factors, Storage.factors + Storage.Size());
    }

    // Value by full index
    size_t Size() const {
        return Storage.Size();
    }
    float* Ptr(size_t i) {
        Y_ASSERT(i <= Storage.Size());
        return factors + i;
    }
    const float* Ptr(size_t i) const {
        Y_ASSERT(i <= Storage.Size());
        return factors + i;
    }
    float& operator[](size_t i) {
        Y_ASSERT(i < Storage.Size());
        return factors[i];
    }
    float operator[](size_t i) const {
        Y_ASSERT(i < Storage.Size());
        return factors[i];
    }
    TArrayRef<float> RawData() {
        return {factors, Size()};
    }
    TArrayRef<const float> RawData() const {
        return {factors, Size()};
    }
    size_t size() const {
        return Size();
    }

    // Create simple views
    TFactorView CreateView() {
        return TFactorView(*this);
    }
    TConstFactorView CreateConstView() const {
        return TConstFactorView(*this);
    }

protected:
    TGeneralResizeableFactorStorage Storage;
};

class TFactorStorage
    : public TBasicFactorStorage
{
    friend TString SerializeFactorBorders(const TFactorStorage& storage);

public:
    // Ctors that allocate TFactorDomain
    TFactorStorage()
        : TBasicFactorStorage(0)
    {}
    template <class TPool>
    TFactorStorage(TPool* pool)
        : TBasicFactorStorage(0, pool)
    {
    }

    explicit TFactorStorage(const TFactorDomain& domain)
        : TBasicFactorStorage(domain.Size())
        , Domain(new (Storage.AllocateDomain()) TFactorDomain(domain))
    {
    }
    template <class TPool>
    TFactorStorage(const TFactorDomain& domain, TPool* pool)
        : TBasicFactorStorage(domain.Size(), pool)
        , Domain(new (Storage.AllocateDomain()) TFactorDomain(domain))
    {
    }

    // Copy & move
    TFactorStorage(const TFactorStorage& other)
        : TBasicFactorStorage(other)
        , Domain(new (Storage.AllocateDomain()) TFactorDomain(*other.Domain))
        , MxNetIndex(other.MxNetIndex)
    {
    }
    template <typename TPool>
    TFactorStorage(const TFactorStorage& other, TPool* pool)
        : TBasicFactorStorage(other, pool)
        , Domain(new (Storage.AllocateDomain()) TFactorDomain(*other.Domain))
        , MxNetIndex(other.MxNetIndex)
    {
    }
    TFactorStorage(TFactorStorage&& other)
        : TBasicFactorStorage(std::move(other))
    {
        DoSwap(Domain, other.Domain);
        DoSwap(MxNetIndex, other.MxNetIndex);
    }
    template <typename TPool>
    TFactorStorage(TFactorStorage&& other, TPool* pool)
        : TBasicFactorStorage(std::move(other), pool)
    {
        DoSwap(Domain, other.Domain);
        DoSwap(MxNetIndex, other.MxNetIndex);
    }
    TFactorStorage& operator=(const TFactorStorage& other) {
        Y_ASSERT(other.Domain);
        TBasicFactorStorage::operator=(other);
        Domain = new (Storage.AllocateDomain()) TFactorDomain(*other.Domain);
        MxNetIndex = other.MxNetIndex;
        return *this;
    }
    TFactorStorage& operator=(TFactorStorage&& other) {
        Y_ASSERT(other.Domain);
        TBasicFactorStorage::operator=(std::move(other));
        DoSwap(Domain, other.Domain);
        MxNetIndex = other.MxNetIndex;
        return *this;
    }
    void Swap(TFactorStorage& other) {
        TBasicFactorStorage::Swap(other);
        DoSwap(Domain, other.Domain);
        DoSwap(MxNetIndex, other.MxNetIndex);
    }

    // Ctors that use shared TFactorDomain
    explicit TFactorStorage(const TFactorDomain* domain)
        : TBasicFactorStorage(domain->Size())
        , Domain(domain)
    {
        Y_ASSERT(Domain);
    }
    template <class TPool>
    TFactorStorage(const TFactorDomain* domain, TPool* pool)
        : TBasicFactorStorage(domain->Size(), pool)
        , Domain(domain)
    {
        Y_ASSERT(Domain);
    }

    // Helper constructors
    explicit TFactorStorage(const TFactorBorders& borders)
        : TFactorStorage(TFactorDomain(borders))
    {
    }
    template <class TPool>
    TFactorStorage(const TFactorBorders& borders, TPool* pool)
        : TFactorStorage(TFactorDomain(borders), pool)
    {
    }
    explicit TFactorStorage(ui32 count)
        : TFactorStorage(TFactorDomain(count))
    {
    }
    template <class TPool>
    TFactorStorage(ui32 count, TPool* pool)
        : TFactorStorage(TFactorDomain(count), pool)
    {
    }
    template <class TPool>
    TFactorStorage(ui32 count, TPool* pool, EFactorUniverse universe)
        : TFactorStorage(TFactorDomain(count, universe), pool)
    {
    }

    const TFactorDomain& GetDomain() const {
        Y_ASSERT(Domain);
        return *Domain;
    }
    const TFactorBorders& GetBorders() const {
        Y_ASSERT(Domain);
        return Domain->GetBorders();
    }
    NFactorSlices::EFactorUniverse GetUniverse() const {
        Y_ASSERT(Domain);
        return Domain->GetUniverse();
    }
    EFactorSlice GetSliceFor(ESliceRole role) const {
        Y_ASSERT(Domain);
        return Domain->GetSliceFor(role);
    }

    // Following methods guarantee that
    // raw array of factor values is preserved.
    // Slice-relative values may change, i.e. in
    //    x0 = storage.CreateViewFor(WEB_PRODUCTION)[0];
    //    storage.SetDomain(newDomain);
    //    x1 = storage.CreateViewFor(WEB_PRODUCTION)[0];
    // it is possible to have x0 != x1
    void SetDomain(const TFactorDomain& domain) {
        Domain = new (Storage.AllocateDomain()) TFactorDomain(domain);
        Storage.Resize(Domain->Size());
        factors = Storage.factors;
    }
    void SetDomain(const TFactorDomain* domain) {
        Y_ASSERT(domain);
        Domain = domain;
        Storage.Resize(Domain->Size());
        factors = Storage.factors;
    }
    void SetBordersFrom(const TFactorStorage& storage) {
        Y_ASSERT(storage.Domain);
        SetDomain(*storage.Domain);
    }
    void SetBorders(const TFactorBorders& borders) {
        SetDomain(TFactorDomain(borders));
    }

    using TBasicFactorStorage::operator[];
    using TBasicFactorStorage::Ptr;

    // Value by full factor index
    float* Ptr(const TFullFactorIndex& index) {
        Y_ASSERT(Domain);
        if (Domain->HasIndex(index)) {
            return factors + Domain->GetIndex(index);
        }
        return nullptr;
    }
    const float* Ptr(const TFullFactorIndex& index) const {
        Y_ASSERT(Domain);
        if (Domain->HasIndex(index)) {
            return factors + Domain->GetIndex(index);
        }
        return nullptr;
    }
    Y_FORCE_INLINE float& operator[](const TFullFactorIndex& index) {
        Y_ASSERT(Ptr(index) != nullptr);
        Y_ASSERT(Ptr(index) < Ptr(0) + Size());
        return *Ptr(index);
    }
    Y_FORCE_INLINE float operator[](const TFullFactorIndex& index) const {
        Y_ASSERT(Ptr(index) != nullptr);
        Y_ASSERT(Ptr(index) < Ptr(0) + Size());
        return *Ptr(index);
    }

    void SetMatrixNetIndex(const TFullFactorIndex& index) {
        MxNetIndex = index;
    }
    float& MatrixNet() {
        return operator[](MxNetIndex);
    }
    float MatrixNet() const {
        return operator[](MxNetIndex);
    }

    void SetMatrixNetFactorIfHas(float value) {
        float* mxNetPtr = Ptr(MxNetIndex);
        if (mxNetPtr) {
            *mxNetPtr = value;
        }
    }

    float GetMatrixNetOrZero() const {
        const float* mxNetPtr = Ptr(MxNetIndex);
        return mxNetPtr ? *mxNetPtr : 0.f;
    }

    // Create slice-aware views
    TFactorView CreateView() {
        return CreateViewFor(EFactorSlice::ALL);
    }
    TConstFactorView CreateConstView() const {
        return CreateConstViewFor(EFactorSlice::ALL);
    }
    TFactorView operator[] (EFactorSlice slice) {
        return CreateViewFor(slice);
    }
    TFactorView CreateViewFor(EFactorSlice slice) {
        Y_ASSERT(Domain);
        return TFactorView(slice, *Domain, factors + (*Domain)[slice].Begin);
    }
    TConstFactorView CreateConstViewFor(EFactorSlice slice) const {
        Y_ASSERT(Domain);
        return TConstFactorView(slice, *Domain, factors + (*Domain)[slice].Begin);
    }

    TFactorView CreateViewForRole(ESliceRole role) {
        return CreateViewFor(GetSliceFor(role));
    }
    TConstFactorView CreateConstViewForRole(ESliceRole role) const {
        return CreateConstViewFor(GetSliceFor(role));
    }

    // Create multi views
    template <typename... Args>
    TMultiFactorView CreateMultiViewFor(Args... args) {
        Y_ASSERT(Domain);
        std::initializer_list<EFactorSlice> slices = {Domain->GetSliceFor(args)...};
        return TMultiFactorView(slices.begin(), slices.end(), *Domain, factors);
    }
    template <typename... Args>
    TMultiConstFactorView CreateMultiConstViewFor(Args... args) const {
        Y_ASSERT(Domain);
        std::initializer_list<EFactorSlice> slices = {Domain->GetSliceFor(args)...};
        return TMultiConstFactorView(slices.begin(), slices.end(), *Domain, factors);
    }

    TArrayRef<const float> GetFactorsRegion(TStringBuf sliceName) const {
        EFactorSlice sliceId = EFactorSlice::COUNT;
        if (TryFromString(sliceName, sliceId)) {
            auto view = CreateConstViewFor(sliceId);
            return {~view, +view};
        }
        return {};
    }

    TVector<EFactorSlice> GetFactorSlices() const {
        return NFactorSlices::GetFactorSlices(GetBorders());
    }

    bool HasFactorSlice(EFactorSlice slice, NFactorSlices::ESerializationMode mode) const {
        return IsIn(NFactorSlices::GetFactorSlices(GetBorders(), mode), slice);
    }

private:
    static const TFactorDomain EmptyDomain;
    const TFactorDomain* Domain = &EmptyDomain;
    TFullFactorIndex MxNetIndex{NFactorSlices::EFactorSlice::WEB_PRODUCTION, 379}; // TODO: remove later. 379 == FI_MATRIXNET for web
};

static inline TArrayRef<const float> GetFactorsRegion(const TFactorStorage* storage, TStringBuf sliceName) {
    return storage->GetFactorsRegion(sliceName);
}

namespace NFSSaveLoad {
    void Serialize(const TFactorStorage& storage, IOutputStream* output);

    void Deserialize(IInputStream* input, const NFactorSlices::TSlicesMetaInfo& hostInfo, TFactorStorage* dst);
    void Deserialize(const TStringBuf& factorBorders, const TStringBuf& huffEncodedFactors,
        const NFactorSlices::TSlicesMetaInfo& hostInfo, TFactorStorage* dst);

    template<typename TPool>
    TFactorStorage* Deserialize(IInputStream* input, const NFactorSlices::TSlicesMetaInfo& hostInfo, TPool* pool) {
        TFactorStorage* dst = CreateFactorStorageInPool(pool);
        Deserialize(input, hostInfo, dst);
        return dst;
    }

    inline THolder<TFactorStorage> Deserialize(IInputStream* input, const NFactorSlices::TSlicesMetaInfo& hostInfo) {
        THolder<TFactorStorage> dst(new TFactorStorage());
        Deserialize(input, hostInfo, dst.Get());
        return dst;
    }

    template<typename T>
    TFactorStorage* Deserialize(const TStringBuf& factorBorders, const TStringBuf& huffEncodedFactors,
        const NFactorSlices::TSlicesMetaInfo& hostInfo, T* pool)
    {
        TFactorStorage* dst = CreateFactorStorageInPool(pool);
        Deserialize(factorBorders, huffEncodedFactors, hostInfo, dst);
        return dst;
    }

    inline THolder<TFactorStorage> Deserialize(const TStringBuf& factorBorders, const TStringBuf& huffEncodedFactors,
        const NFactorSlices::TSlicesMetaInfo& hostInfo)
    {
        THolder<TFactorStorage> dst(new TFactorStorage());
        Deserialize(factorBorders, huffEncodedFactors, hostInfo, dst.Get());
        return dst;
    }
} // namespace NFSSaveLoad

inline bool ValidAndHasFactors(const TBasicFactorStorage* storage) {
    return storage && storage->factors;
}

struct TFactorStorageRefCounted
    : public TFactorStorage
{
    int RefCount = 1;

    template <typename... Args>
    TFactorStorageRefCounted(Args... args)
        : TFactorStorage(args...)
    {}
};

template <size_t FACTOR_COUNT>
struct TGeneralFactorStorage
    : public TGeneralResizeableFactorStorage
{
    TGeneralFactorStorage()
        : TGeneralResizeableFactorStorage(FACTOR_COUNT)
    {
    }
};

template <typename TStorage>
inline size_t GetFactorStorageInPoolSizeBound(ui32 count) {
    return sizeof(TStorage) // Bytes for storage object
        + sizeof(float) * count // Bytes for factors array
        + 64; // Misc overhead estimate
}

template <>
inline size_t GetFactorStorageInPoolSizeBound<TFactorStorage>(ui32 count) {
    return sizeof(TFactorStorage) // Bytes for storage object
        + sizeof(float) * count // Bytes for factors array
        + sizeof(TFactorDomain) // Bytes for TFactorDomain object
        + 64; // Misc overhead estimate
}

namespace NFSSaveLoad {
struct TAppendFeaturesResult {
    TVector<float> Features;
    TString Borders;
    size_t AppendedFeaturesBegin = 0;
    size_t AppendedFeaturesEnd = 0;
};

TAppendFeaturesResult AppendFeaturesToSlice(
    TArrayRef<const float> inputFeatures,
    TStringBuf inputSliceString,
    EFactorSlice sliceToAppendInto,
    TArrayRef<const float> newFeatures,
    bool skipSlicesAndFeatsMissmatch = false) noexcept(false);

} //namesapce NFSSaveLoad
