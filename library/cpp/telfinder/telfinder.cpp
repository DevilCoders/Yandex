#include "telfinder.h"

#include <library/cpp/solve_ambig/occ_traits.h>
#include <library/cpp/solve_ambig/solve_ambiguity.h>

#include <util/charset/unidata.h>
#include <util/generic/singleton.h>
#include <library/cpp/containers/sorted_vector/sorted_vector.h>

namespace {
    const TString BAD_CODES[] = {"2", "21", "22", "23", "24", "25", "26", "29", "3", "35", "37", "38",
                                        "4", "42", "5", "50", "59", "6", "67", "68", "69", "80", "85", "87", "88", "9", "96", "97", "99"};

    struct TBadCountryCodes {
        NSorted::TSortedVector<TString> BadCountryCodes;

        TBadCountryCodes() {
            for (const auto& i : BAD_CODES)
                BadCountryCodes.insert(i);
        }

        void FixPhoneCountryCode(TString& countryCode, TString& areaCode) const {
            if (countryCode == "0" || countryCode == "00") {
                countryCode.remove();
                return;
            }

            for (size_t i = 0; i < 2; ++i) {
                if (BadCountryCodes.has(countryCode) && areaCode.length() > 1) {
                    countryCode.append(areaCode[0]);
                    areaCode = areaCode.remove(0, 1);
                }
            }
        }
    };

    using TChunkIter = TVector<TTelephonoidChunk>::const_iterator;

    inline bool Shift(size_t& offset, size_t& nextOffset, TChunkIter& chunkIter) {
        if (offset + chunkIter->Token.length() <= nextOffset) {
            offset += chunkIter->Token.length();
            ++chunkIter;
            return true;
        }
        return false;
    }

} // unnamed namespace

namespace NSolveAmbig {
    template <>
    struct TOccurrenceTraits<std::pair<TTelephonoid, TAreaScheme>> {
        inline static TStringBuf GetId(const std::pair<TTelephonoid, TAreaScheme>&) {
            return TStringBuf();
        }
        inline static size_t GetCoverage(const std::pair<TTelephonoid, TAreaScheme>& occ) {
            return GetStop(occ) - GetStart(occ);
        }
        inline static size_t GetStart(const std::pair<TTelephonoid, TAreaScheme>& occ) {
            return occ.first.Chunks.front().TokenPos;
        }
        inline static size_t GetStop(const std::pair<TTelephonoid, TAreaScheme>& occ) {
            return occ.first.Chunks.back().TokenPos + 1;
        }
        inline static double GetWeight(const std::pair<TTelephonoid, TAreaScheme>&) {
            return 0.0;
        }
    };

}

////
//  TTelFinder
////

bool TTelFinder::IsNumber(TWtringBuf token) const {
    if (token.size() == 0) {
        return false;
    }
    for (const wchar16* it = token.data(); it < token.data() + token.size(); ++it) {
        if (!IsCommonDigit(*it)) {
            return false;
        }
    }
    return true;
}

template <>
void TTelFinder::ShiftStates<true>(const TTelephonoidChunk& tc, TVector<TSearchState>& states, TString&) {
    for (TSearchState& state : states) {
        state.Id.Chunks.push_back(tc);
    }
}

template <>
void TTelFinder::ShiftStates<false>(const TTelephonoidChunk& tc, TVector<TSearchState>& states, TString& buff) {
    buff.clear();
    buff.reserve(4);
    TTelephonoidChunk::SpaceToString(buff, tc.PrecedingSpacesTypes);

    // Advance states using preceding delimiters
    if (!buff.empty()) {
        TVector<TSearchState>::iterator iState = states.begin();
        while (iState != states.end()) {
            if (iState->Iter.Advance(buff.data(), buff.size())) {
                ++iState;
            } else {
                iState = states.erase(iState);
            }
        }
    }

    // For each digit try to advance states using digit itself or using common placeholder ('#')
    for (size_t i = 0; i < tc.Token.length() && !states.empty(); ++i) {
        size_t stop = states.size();
        size_t s = 0;
        while (s < stop) {
            TSearchIterator<TPhoneSchemes::TSchemeTrie> iter = states[s].Iter;
            if (states[s].Iter.Advance(tc.Token[i])) {
                if (iter.Advance('#')) {
                    TSearchState newState = {states[s].Id, iter, states[s].CommulativeSpacesTypes};
                    states.push_back(newState);
                }
                ++s;
            } else if (iter.Advance('#')) {
                states[s].Iter = iter;
                ++s;
            } else {
                states.erase(states.begin() + s);
                --stop;
            }
        }
    }

    // Add chunk to succesdfully shifted states
    for (size_t i = 0; i < states.size(); ++i) {
        states[i].Id.Chunks.push_back(tc);
    }
}

template <bool ignoreSchemes>
bool TTelFinder::CheckStates(TVector<TSearchState>& states, TVector<TResultState>& res, ui16 spaces) {
    TAreaScheme scheme;
    bool phoneBoundary = false;
    for (auto& state : states) {
        if (Y_LIKELY(res.size() < MAX_SEQUENTIAL_TELEPHONEIDS)) { // Too many sequential phones is most likely a trash
            // Check potential phone boundaries using "+", "(", punctuation or spaces.
            // Additionaly check for space boundary that previous numbers have any delimiters except spaces
            if ((spaces & WEAK_SEP) || ((spaces & TTelephonoidChunk::DSpace) && (state.CommulativeSpacesTypes & NON_SP))) {
                if ((ignoreSchemes && state.Id.HasGoodDelimiters()) || state.Iter.GetValue(&scheme)) {
                    res.push_back(std::make_pair(state.Id, scheme));
                    phoneBoundary = true;
                }
            }
        }

        state.CommulativeSpacesTypes |= spaces;
    }
    return phoneBoundary;
}

template bool TTelFinder::CheckStates<true>(TVector<TSearchState>& states, TVector<TResultState>& res, ui16 spaces);
template bool TTelFinder::CheckStates<false>(TVector<TSearchState>& states, TVector<TResultState>& res, ui16 spaces);

void TTelFinder::FlushResults(TVector<TResultState>& res, IPhoneProcessor& phoneProcessor) {
    if (res.size() > 1) {
        NSolveAmbig::SolveAmbiguity(res);
    }
    for (const auto& re : res) {
        phoneProcessor.ProcessTelephonoid(re.first, re.second);
    }
    res.clear();
}

TPhone TTelFinder::CreatePhone(const TTelephonoid& telephonoid, const TAreaScheme& scheme) {
    size_t offset = 0;
    TString fullPhone = telephonoid.FullNumberToString();
    offset += scheme.CountryOffset;
    TString countryCode(fullPhone, offset, scheme.CountryLength);
    offset += scheme.CountryLength + scheme.AreaOffset;
    TString areaCode(fullPhone, offset, scheme.AreaLength);
    offset += scheme.AreaLength;
    TString localPhone(fullPhone, offset, fullPhone.length() - offset);
    Default<TBadCountryCodes>().FixPhoneCountryCode(countryCode, areaCode);
    return TPhone(countryCode, areaCode, localPhone);
}

TPhoneLocation TTelFinder::CreateLocation(const TTelephonoid& telephonoid, const TAreaScheme& scheme) {
    TPhoneLocation location;
    TChunkIter chunkIter = telephonoid.Chunks.begin();
    size_t nextOffset = 0;
    size_t offset = 0;

    //skipping to country code
    nextOffset += scheme.CountryOffset;
    while (offset < nextOffset) {
        if (!Shift(offset, nextOffset, chunkIter))
            break;
    }
    //parsing country code
    nextOffset += scheme.CountryLength;
    while (offset < nextOffset) {
        if (location.CountryCodePos.first == location.CountryCodePos.second) {
            location.CountryCodePos.first = chunkIter->TokenPos;
        }
        location.CountryCodePos.second = chunkIter->TokenPos + 1;
        if (!Shift(offset, nextOffset, chunkIter))
            break;
    }
    //skipping to area code
    nextOffset += scheme.AreaOffset;
    while (offset < nextOffset) {
        if (!Shift(offset, nextOffset, chunkIter))
            break;
    }
    //parsing area code
    nextOffset += scheme.AreaLength;
    while (offset < nextOffset) {
        if (location.AreaCodePos.first == location.AreaCodePos.second) {
            location.AreaCodePos.first = chunkIter->TokenPos;
        }
        location.AreaCodePos.second = chunkIter->TokenPos + 1;
        if (!Shift(offset, nextOffset, chunkIter))
            break;
    }
    //parsing local phone
    while (chunkIter != telephonoid.Chunks.end()) {
        if (location.LocalPhonePos.first == location.LocalPhonePos.second) {
            location.LocalPhonePos.first = chunkIter->TokenPos;
        }
        location.LocalPhonePos.second = chunkIter->TokenPos + 1;
        chunkIter++;
    }
    if (scheme.CountryLength > 0) {
        location.PhonePos.first = location.CountryCodePos.first;
    } else if (scheme.AreaLength > 0) {
        location.PhonePos.first = location.AreaCodePos.first;
    } else {
        location.PhonePos.first = location.LocalPhonePos.first;
    }
    location.PhonePos.second = telephonoid.Chunks.back().TokenPos + 1;
    return location;
}

TFoundPhone TTelFinder::CreateFoundPhone(const TTelephonoid& telephonoid, const TAreaScheme& scheme) {
    TFoundPhone foundPhone;
    foundPhone.Structure = telephonoid.StructureToString();
    foundPhone.Phone = CreatePhone(telephonoid, scheme);
    foundPhone.Location = CreateLocation(telephonoid, scheme);
    return foundPhone;
}
