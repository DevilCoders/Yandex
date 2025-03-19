#include "leb128.h"
#include "binarized_word_hash_vector.h"

#include <library/cpp/vowpalwabbit/vowpal_wabbit_predictor.h>

#include <util/generic/map.h>
#include <util/stream/str.h>

#include <cmath>

#ifdef __linux__
    #include <alloca.h>
#endif

NClickSim::TBinarizedWordHashVector::TBinarizedWordHashVector(const THashMap<TString, double>& word2weight) {
    Hashes.reserve(word2weight.size());

    for (const auto& p : word2weight) {
        Y_ASSERT(p.second >= 0 && p.second <= 1.0);
        const ui32 weight = lround(p.second * MaxWeight);
        Y_ASSERT(weight >= 0 && weight <= MaxWeight);
        if (weight > 0) {
            const ui32 hash = NVowpalWabbit::HashString(p.first.data(), p.first.data() + p.first.size(), 0);
            Hashes.emplace_back(hash, weight);
        }
    }

    std::sort(Hashes.begin(), Hashes.end());

    // merge duplicated hashes
    size_t first = 0;
    size_t second = 1;
    size_t gap = 0;
    for (; second < Hashes.size(); ++first, ++second) {
        if (Y_UNLIKELY(Hashes[first].Hash == Hashes[second].Hash)) {
            for (;;) {
                const ui32 weight = std::min(static_cast<ui32>(Hashes[first].Weight) + Hashes[second].Weight, MaxWeight);
                Hashes[first].Weight = weight;
                ++second;
                ++gap;
                if (second >= Hashes.size() || Hashes[first].Hash != Hashes[second].Hash) {
                    break;
                }
            }
        }

        if (Y_UNLIKELY(gap > 0) && second < Hashes.size()) {
            Hashes[second - gap] = Hashes[second];
        }
    }

    if (Y_UNLIKELY(gap > 0)) {
        Hashes.resize(Hashes.size() - gap);
    }
}

NClickSim::TBinarizedWordHashVector::TBinarizedWordHashVector(const TMap<ui32, ui8>& sortedHashes) {
    Hashes.reserve(sortedHashes.size());

    for (const auto& p : sortedHashes) {
        Y_ASSERT(p.second > 0);
        Hashes.emplace_back(p.first, p.second);
    }
}

ui64 NClickSim::TBinarizedWordHashVector::operator*(const NClickSim::TBinarizedWordHashVector& other) const {
    return DotProduct(other.Hashes.begin(), other.Hashes.end());
}

ui64 NClickSim::TBinarizedWordHashVector::DotProduct(const NClickSim::THashWithWeight* begin,
                                                     const NClickSim::THashWithWeight* end) const
{
    auto it1 = Hashes.data();
    auto end1 = it1 + Hashes.size();
    if (Y_UNLIKELY(it1 == end1 || begin == end)) {
        return 0;
    }

    auto it2 = begin;
    ui64 result = 0;
    for (;;) {
        if (it1->Hash == it2->Hash) {
            result += static_cast<ui32>(it1->Weight) * it2->Weight;
            ++it1;
            if (it1 == end1) {
                return result;
            }
            ++it2;
            if (it2 == end) {
                return result;
            }
        } else if (it1->Hash < it2->Hash) {
            ++it1;
            if (it1 == end1) {
                return result;
            }
        } else {
            Y_ASSERT(it1->Hash > it2->Hash);
            ++it2;
            if (it2 == end) {
                return result;
            }
        }
    }
}

bool NClickSim::TBinarizedWordHashVector::operator==(const NClickSim::TBinarizedWordHashVector &other) const {
    return Hashes.size() == other.Hashes.size() &&
           memcmp(Hashes.data(), other.Hashes.data(), sizeof(THashWithWeight) * Hashes.size()) == 0;

}

void NClickSim::TBinarizedWordHashVector::Save(IOutputStream& os) const {
    os.Write(Hashes.data(), sizeof(THashWithWeight) * Hashes.size());
}

TString NClickSim::TBinarizedWordHashVector::SaveToString() const {
    TString s;
    {
        TStringOutput out(s);
        Save(out);
    }
    return s;
}

TString NClickSim::TBinarizedWordHashVector::SaveToLeb128() const {
    ui8* const start = reinterpret_cast<ui8*>(alloca(Hashes.size() * sizeof(THashWithWeight)));
    ui8* end = start;
    ui32 prev = 0;
    for (const THashWithWeight& h : Hashes) {
        Y_ASSERT(prev == 0 || h.Hash > prev);
        end = WriteLeb128(h.Hash - prev, end);
        *end = h.Weight;
        ++end;
        prev = h.Hash;
    }
    return TString(reinterpret_cast<const char*>(start), end - start);
}

void NClickSim::TBinarizedWordHashVector::LoadFromLeb128(TStringBuf s) {
    const ui8* p = reinterpret_cast<const ui8*>(s.data());
    const ui8* const end = p + s.size();
    Hashes.clear();
    ui32 prev = 0;
    while (p < end) {
        THashWithWeight h;
        p = ReadLeb128(h.Hash, p);
        Y_ASSERT( h.Hash > 0 || Hashes.empty());
        h.Hash += prev;
        prev = h.Hash;
        h.Weight = *p;
        ++p;
        Hashes.emplace_back(h);
    }

    Y_ENSURE(p == end, "Serialization data is invalid");
}

void NClickSim::TBinarizedWordHashVector::Load(TArrayRef<const char> data) {
    Y_ASSERT(data.size() % sizeof(THashWithWeight) == 0);
    Hashes.yresize(data.size() / sizeof(THashWithWeight));
    memcpy(Hashes.data(), data.data(), sizeof(THashWithWeight) * Hashes.size());
}
