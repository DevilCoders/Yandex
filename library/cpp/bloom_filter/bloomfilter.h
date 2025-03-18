#pragma once

#include <library/cpp/pop_count/popcount.h>

#include <util/system/defaults.h>
#include <util/generic/ymath.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/generic/algorithm.h>
#include <util/generic/string.h>
#include <util/generic/array_ref.h>

class IInputStream;
class IOutputStream;
struct TMappedBloomFilter;

// compact 'set' data structure, with controlled false positive probability
// for information about math and restrictions, see http://en.wikipedia.org/wiki/Bloom_filter

struct TBloomFilterHeader {
    const static size_t HeaderLoadSize = 2 * sizeof(ui64);
    size_t HashCount = 0;
    size_t BitCount = 0;

    ///versions:
    //0: old version with very non-orthogonal hash function, which cause collision on simple input
    //1: new version with fixed hash-function
    //2: new version with fixed hash-function and faster ( EFasterVersion { E_FASTER } );
    ui32 HashVersion = 1;

    void Save(IOutputStream* out) const;
    void Load(IInputStream* inp);
};

class TBloomFilter: private TBloomFilterHeader {
    friend struct TMappedBloomFilter;

public:
    static size_t GetBitCount(size_t n, float e) {
        Y_ENSURE(0.0 < e && e < 1.0, "invalid TBloomFilter error parameter " << e);
        Y_ENSURE(n > 0, "invalid TBloomFilter elements count parameter " << n);

        double fractionalBitCount = -(n * log(e) / pow(log(2.0f), 2));

        return static_cast<size_t>(fractionalBitCount);
    }

    static size_t GetBitCount(size_t n, size_t k, float e) {
        Y_ENSURE(0.0 < e && e < 1.0, "invalid TBloomFilter error parameter " << e);
        Y_ENSURE(n > 0, "invalid TBloomFilter elements count parameter " << n);
        Y_ENSURE(k > 0, "invalid TBloomFilter hashes count parameter " << k);

        return static_cast<size_t>(ceil(-1.0 / log(1.0 - pow(e, 1.0 / k)) * (k * n)));
    }

    static size_t GetHashCount(float e) {
        Y_ENSURE(0.0 < e && e < 1.0, "invalid TBloomFilter error parameter " << e);

        return static_cast<size_t>(ceil(-Log2(e)));
    }

    TBloomFilter() {
        SetHashFuncPtr(this);
    }

    //round BitCount (memory usage) to power of 2, works 30-50% faster.
    enum EFasterVersion { E_FASTER };
    TBloomFilter(size_t elemcount, float error, EFasterVersion);

    TBloomFilter(size_t elemcount, float error)
        : TBloomFilter(TInternalTag{},
                       std::max(size_t(1), GetHashCount(error)),
                       std::max(size_t(64), GetBitCount(std::max(elemcount, size_t(1)), error))) {
    }

    // You can specify number of hash functions explicitly.
    // As a result bloom filter will be faster, but will use a much more memory
    TBloomFilter(size_t elemcount, size_t hashcount, float error)
        : TBloomFilter(TInternalTag{}, hashcount, std::max(size_t(64), GetBitCount(std::max(elemcount, size_t(1)), hashcount, error)))
    {
    }

    size_t GetBitCount() const {
        return BitCount;
    }

    size_t GetHashCount() const {
        return HashCount;
    }

    void Add(const TString& val) {
        Add(val.data(), val.size());
    }

    template <typename TStringType>
    void Add(const TStringType& val) {
        Add(val.data(), val.length() * sizeof(typename TStringType::TChar));
    }

    void Add(const void* val, size_t len) {
        (*AddShPtr)(this, val, len);
    }

    bool Has(const TString& val) const {
        return Has(val.data(), val.size());
    }

    template <typename TStringType>
    bool Has(const TStringType& val) const {
        return Has(val.data(), val.length() * sizeof(typename TStringType::TChar));
    }

    bool Has(const void* val, size_t len) const {
        return (*HasShPtr)(this, val, len);
    }

    float EstimateItemNumber() const {
        size_t count = BitmapOneBitsCount();

        if (count == BitCount) {
            return -(BitCount * log(0.001f)) / ((float)HashCount);
        } else {
            return -(BitCount * log(1.0f - count / ((float)BitCount))) / ((float)HashCount);
        }
    }

    void Clear() {
        Fill(Vec.begin(), Vec.end(), 0);
    }

    bool EmptyBitmap() const {
        return AllOf(Vec, [](ui64 x) { return x == 0; });
    }

    size_t BitmapBitsSize() const {
        return BitCount;
    }

    size_t BitmapOneBitsCount() const {
        return Accumulate(Vec, size_t(0), [](size_t count, ui64 x) { return count + (size_t)PopCount(x); });
    }

    TBloomFilter& Union(const TBloomFilter& other) {
        return BinaryOp(other, [](ui64* d, ui64 s) { *d |= s; });
    }

    TBloomFilter& SetDifference(const TBloomFilter& other) {
        return BinaryOp(other, [](ui64* d, ui64 s) { *d &= ~s; });
    }

    TBloomFilter& Intersection(const TBloomFilter& other) {
        return BinaryOp(other, [](ui64* d, ui64 s) { *d &= s; });
    }

    template <class F>
    TBloomFilter& BinaryOp(const TBloomFilter& other, F f) {
        Y_ENSURE(HashCount == other.HashCount, "Hash count is not the same");
        Y_ENSURE(BitCount == other.BitCount, "Bit count is not the same");
        Y_ENSURE(HashVersion == other.HashVersion, "Hash version is not the same");
        Y_ENSURE(Vec.size() == other.Vec.size(), "different sizes of vectors" << Vec.size() << ' ' << other.Vec.size());

        ui64* dst = Vec.data();
        const ui64* src = other.Vec.data();

        for (size_t i = 0, iEnd = Vec.size(); i != iEnd; ++i) {
            f(&dst[i], src[i]);
        }

        return *this;
    }

    void Save(IOutputStream* out) const;
    void Load(IInputStream* inp);
    void SafeLoad(IInputStream* inp);

private:
    struct TInternalTag {}; // to avoid disambiguation with public constructors

    TBloomFilter(TInternalTag, size_t hashcount, size_t bitcount) {
        HashCount = hashcount;
        BitCount = bitcount;
        Reserve();
        SetHashFuncPtr(this);
    }

    template <class TStore>
    static void SetHashFuncPtr(TStore* me);

    template <class TStore, class THashGenerator>
    static void AddSHTemplate(TStore* me, const void* val, size_t len);

    template <class TStore, class THashGenerator>
    static bool HasSHTemplate(const TStore* me, const void* val, size_t len);

    void Reserve() {
        Vec.resize((BitCount + 63) / 64);
    }

    void Load(IInputStream* inp, bool isSafe);

private:
    void (*AddShPtr)(TBloomFilter* me, const void* val, size_t len);
    bool (*HasShPtr)(const TBloomFilter* me, const void* val, size_t len);
    TVector<ui64> Vec;
};

/// rounds memory size to power of 2, works faster.
class TBloomFilterFaster: public TBloomFilter {
public:
    TBloomFilterFaster(size_t elemcount, float error)
        : TBloomFilter(elemcount, error, E_FASTER)
    {
    }
    TBloomFilterFaster() = default;
};

struct TMappedBloomFilter: public TBloomFilterHeader {
    TArrayRef<ui64> Vec;

    inline TMappedBloomFilter() noexcept = default;

    inline TMappedBloomFilter(TBloomFilter& bf) noexcept
        : TBloomFilterHeader(bf)
        , Vec(bf.Vec.data(), bf.Vec.size())
    {
        Init();
    }

    inline TMappedBloomFilter(const TBloomFilterHeader& bh, TArrayRef<ui64> vec) noexcept
        : TBloomFilterHeader(bh)
        , Vec(vec)
    {
        Init();
    }

    inline bool Has(const TString& val) const noexcept {
        return Has(val.data(), val.size());
    }

    template <typename TStringType>
    inline bool Has(const TStringType& val) const noexcept {
        return Has(val.data(), val.length() * sizeof(typename TStringType::TChar));
    }

    inline bool Has(const void* val, size_t len) const noexcept {
        return (*HasShPtr)(this, val, len);
    }

private:
    friend class TBloomFilter;

    void Init();

    void (*AddShPtr)(TMappedBloomFilter* me, const void* val, size_t len) = nullptr;
    bool (*HasShPtr)(const TMappedBloomFilter* me, const void* val, size_t len) = nullptr;
};
