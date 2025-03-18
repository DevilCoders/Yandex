#include "bloomfilter.h"

#include <util/digest/murmur.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/ysaveload.h>

void TBloomFilterHeader::Save(IOutputStream* out) const {
    Y_ENSURE((ui64(HashCount) >> 32) == 0, "TBloomFilterHeader::Save: unexpected large HashCount");
    ui64 tmp = HashCount + (ui64(HashVersion) << 32);
    ::Save(out, ui64(tmp));
    ::Save(out, ui64(BitCount));
}

void TBloomFilterHeader::Load(IInputStream* inp) {
    ui64 count = 0;
    ::Load(inp, count);
    HashCount = size_t(count & 0xFFFFFFFF);
    HashVersion = ui32(count >> 32);
    ::Load(inp, count);
    BitCount = size_t(count);
}

struct THashGenerator_Version0 {
    const size_t BitCount;
    size_t HashFuncIndex;

    THashGenerator_Version0(size_t bitCount)
        : BitCount(bitCount)
        , HashFuncIndex(0)
    {
    }

    size_t GetBits(const void* val, size_t len) {
        return MurmurHash(val, len, HashFuncIndex++) % BitCount;
    }
};

struct THashGenerator_Version1 {
    const size_t BitCount;
    size_t HashFuncIndex;

    THashGenerator_Version1(size_t bitCount)
        : BitCount(bitCount)
        , HashFuncIndex(0)
    {
    }

    size_t GetBits(const void* val, size_t len) {
        static size_t randomSeeds[] = {
            // http://clubs.at.yandex-team.ru/arcadia/4664
            size_t(0x4b7db4c869874dd1),
            size_t(0x43e9b39115fd33ba),
            size_t(0x180be656098797e4),
            size_t(0xe21f17e9d2d0bae7),
            size_t(0xeaa42039facc7152),
            size_t(0x0d3666daa04ff2fd),
            size_t(0xcafb2c5b513bc4f0),
            size_t(0xdbb86c8c0293d7be),
            size_t(0xec978c08a6a50237),
            size_t(0xa3601812c4207d5d),
            size_t(0x6ad826554038feae),
            size_t(0xebe6d4db55c4f77a),
            size_t(0xe976cceb2abbc306),
            size_t(0xeac2796bf5c2907b),
            size_t(0x0673b5ce5e1e40fd),
            size_t(0x004e6cc070604495),
        };
        size_t seed = randomSeeds[HashFuncIndex % Y_ARRAY_SIZE(randomSeeds)] + HashFuncIndex;
        ++HashFuncIndex;
        return MurmurHash(val, len, seed) % BitCount;
    }
};

template <unsigned NeedHashBits>
struct THashGenerator_Version2 {
    size_t HashFuncIndex;
    size_t Bits;
    size_t HaveBits;

    THashGenerator_Version2(size_t bitCount)
        : HashFuncIndex(0)
        , Bits(0)
        , HaveBits(0)
    {
        Y_ENSURE(bitCount == (ui64(1) << NeedHashBits), "invalid bitCount");
        Y_ENSURE(bitCount > 0, "invalid bitCount");
        Y_ENSURE(NeedHashBits < 64, "NeedHashBits too large ");
    }

    size_t GetBits(const void* val, size_t len) {
        const ui64 mask = ((ui64(1) << NeedHashBits) - 1);
        static size_t randomSeeds[] = {
            // http://clubs.at.yandex-team.ru/arcadia/4664
            size_t(0x4b7db4c869874dd1),
            size_t(0x43e9b39115fd33ba),
            size_t(0x180be656098797e4),
            size_t(0xe21f17e9d2d0bae7),
            size_t(0xeaa42039facc7152),
            size_t(0x0d3666daa04ff2fd),
            size_t(0xcafb2c5b513bc4f0),
            size_t(0xdbb86c8c0293d7be),
            size_t(0xec978c08a6a50237),
            size_t(0xa3601812c4207d5d),
            size_t(0x6ad826554038feae),
            size_t(0xebe6d4db55c4f77a),
            size_t(0xe976cceb2abbc306),
            size_t(0xeac2796bf5c2907b),
            size_t(0x0673b5ce5e1e40fd),
            size_t(0x004e6cc070604495),
        };
        ui64 result;
        if (NeedHashBits <= HaveBits) {
            HaveBits -= NeedHashBits;
            result = Bits & mask;
            Bits >>= NeedHashBits;
        } else {
            result = Bits;
            size_t used = HaveBits;
            size_t needAlso = NeedHashBits - HaveBits;
            size_t seed = randomSeeds[HashFuncIndex % Y_ARRAY_SIZE(randomSeeds)] + HashFuncIndex;
            Bits = MurmurHash(val, len, seed); //note: different versions (and seeds) used on 32/64 compilers

            ++HashFuncIndex;
            HaveBits = 64;

            result = (result | (Bits << used)) & mask;
            Bits >>= needAlso;
            HaveBits -= needAlso;
        }
        return result;
    }
};

template <>
struct THashGenerator_Version2<0> {
    THashGenerator_Version2(size_t bitCount) {
        Y_ENSURE(bitCount == 1, "invalid bitCount");
    }

    size_t GetBits(const void*, size_t) {
        return 0;
    }
};

static void SetBit(ui64* p, ui64 h) {
    p[(h >> 6)] |= (ui64(1) << (h & 63));
}

static ui64 GetBit(const ui64* p, ui64 h) {
    return p[(h >> 6)] & (ui64(1) << (h & 63));
}

TBloomFilter::TBloomFilter(size_t elemcount, float error, TBloomFilter::EFasterVersion) {
    if (elemcount == 0) {
        elemcount = 1; //avoid crashes and zero division
    }
    Y_ENSURE(0.0 < error && error < 1.0, "invalid TBloomFilter error parameter " << error);
    Y_ENSURE(elemcount > 0, "invalid TBloomFilter elements count parameter " << elemcount);
    double fractionalBitCount = -(elemcount * log(error) / pow(log(2.0f), 2));
    // we allocate one ui64 at least, so use 64 bits. This is also improve approximation formulas,
    // which fail on small BitCount
    fractionalBitCount = std::max(64.0, fractionalBitCount);
    BitCount = size_t(1) << size_t(ceil(Log2(fractionalBitCount)));
    HashCount = ceil(std::max(1.0, BitCount / elemcount * log(2.0)));

    HashVersion = 2;

    Reserve();
    SetHashFuncPtr(this);
}

template <class TStore, class THashGenerator>
void TBloomFilter::AddSHTemplate(TStore* me, const void* val, size_t len) {
    Y_ASSERT(me->HashCount > 0);
    THashGenerator hgen(me->BitCount);
    ui64* p = me->Vec.data();
    ui64 h = hgen.GetBits(val, len);
    if (me->HashCount == 1) {
        Y_ASSERT(h < me->BitCount && h < me->Vec.size() * 64);
        SetBit(p, h);
        return;
    }
    for (size_t i = 1, iEnd = me->HashCount; i < iEnd; ++i) {
        Y_PREFETCH_WRITE(p + (h >> 6), 0); //write, don't keep in cache
        Y_ASSERT(h < me->BitCount && h < me->Vec.size() * 64);
        ui64 nextHash = hgen.GetBits(val, len);
        SetBit(p, h);
        h = nextHash;
    }
    SetBit(p, h);
}

template <class TStore, class THashGenerator>
bool TBloomFilter::HasSHTemplate(const TStore* me, const void* val, size_t len) {
    Y_ASSERT(me->HashCount > 0);
    THashGenerator hgen(me->BitCount);
    const ui64* p = me->Vec.data();
    ui64 h = hgen.GetBits(val, len);
    if (me->HashCount == 1) {
        Y_ASSERT(h < me->BitCount && h < me->Vec.size() * 64);
        return GetBit(p, h);
    }
    for (size_t i = 1, iEnd = me->HashCount; i < iEnd; ++i) {
        Y_PREFETCH_READ(p + (h >> 6), 0); //read, don't keep in cache
        ui64 nextHash = hgen.GetBits(val, len);
        Y_ASSERT(h < me->BitCount && h < me->Vec.size() * 64);
        if (!GetBit(p, h)) {
            //Cout << i << ' ' << NeedHashBits << ' ' << h << " here\n";
            return false;
        }
        h = nextHash;
    }
    return GetBit(p, h);
}

template <class TStore>
void TBloomFilter::SetHashFuncPtr(TStore* me) {
    if (me->BitCount == 0) {
        me->AddShPtr = nullptr;
        me->HasShPtr = nullptr;
        return;
    }

    if (me->HashVersion == 0) {
        me->AddShPtr = AddSHTemplate<TStore, THashGenerator_Version0>;
        me->HasShPtr = HasSHTemplate<TStore, THashGenerator_Version0>;
    } else if (me->HashVersion == 1) {
        me->AddShPtr = AddSHTemplate<TStore, THashGenerator_Version1>;
        me->HasShPtr = HasSHTemplate<TStore, THashGenerator_Version1>;
    } else if (me->HashVersion == 2) {
#define _(n)                                                              \
    case n:                                                               \
        me->AddShPtr = AddSHTemplate<TStore, THashGenerator_Version2<n>>; \
        me->HasShPtr = HasSHTemplate<TStore, THashGenerator_Version2<n>>; \
        break;

        Y_ENSURE(me->BitCount > 0, "invalid BitCount " << me->BitCount);
        ui64 needHashBits = (me->BitCount == 1) ? 0 : GetValueBitCount(me->BitCount - 1);
        Y_ENSURE((ui64(1) << needHashBits) == me->BitCount, "BitCount/NeedHashBits mismatch " << needHashBits << ' ' << me->BitCount);

        // clang-format off
        switch (needHashBits) {
            _( 0) _( 1) _( 2) _( 3) _( 4) _( 5) _( 6) _( 7)
            _( 8) _( 9) _(10) _(11) _(12) _(13) _(14) _(15)
            _(16) _(17) _(18) _(19) _(20) _(21) _(22) _(23)
            _(24) _(25) _(26) _(27) _(28) _(29) _(30) _(31)
            _(32) _(33) _(34) _(35) _(36) _(37) _(38) _(39)
            _(40) _(41) _(42) _(43) _(44) _(45) _(46) _(47)
            _(48) _(49) _(50) _(51) _(52) _(53) _(54) _(55)
            _(56) _(57) _(58) _(59) _(60) _(61) _(62) _(63)
            default:
                ythrow yexception() << "unpected BitCount=" << me->BitCount << " with number of bits=" << needHashBits;
        }
// clang-format on
#undef _
    } else {
        ythrow yexception() << "unexpected HashVersion" << me->HashVersion;
    }
}

void TBloomFilter::Save(IOutputStream* out) const {
    Y_ENSURE((ui64(HashCount) >> 32) == 0, "TBloomFilter::Save: unexpected large HashCount");
    TBloomFilterHeader::Save(out);
    ::Save(out, ui8(sizeof(ui64))); //compatibility
    ::Save(out, ui64(Vec.size() * 64));   //compatibility
    ::SavePodArray(out, Vec.data(), Vec.size());
}

void TBloomFilter::Load(IInputStream* inp) {
    Load(inp, false);
}

void TBloomFilter::SafeLoad(IInputStream* inp) {
    Load(inp, true);
}

void TBloomFilter::Load(IInputStream* inp, bool isSafe) {
    TBloomFilterHeader::Load(inp);
    Reserve();

    //this field is not used, compatibility with old saves
    ui8 chunkSize = 0;
    ::Load(inp, chunkSize);
    if (isSafe) {
        Y_ENSURE(size_t(chunkSize) == sizeof(ui64), "Chunk size is not the same");
    } else {
        Y_VERIFY(size_t(chunkSize) == sizeof(ui64), "Chunk size is not the same");
    }

    //this field is not used, compatibility with old saves
    ui64 bitCountOld = 0;
    ::Load(inp, bitCountOld);
    Y_UNUSED(bitCountOld);

    ::LoadPodArray(inp, Vec.data(), Vec.size());
    SetHashFuncPtr(this);
}

void TMappedBloomFilter::Init() {
    TBloomFilter::SetHashFuncPtr(this);
}
