//
// Created by Mikhail Yutman on 30.03.2020.
//

#include "structures.h"

int TText::Size() const {
    return Words.size();
}

TString TText::JoinWithWhitespaces(ui32 l, ui32 r) const {
    TStringBuilder builder{};
    for (ui32 i = l; i < r; i++) {
        builder << Words[i];
        if (i + 1 != r) {
            builder << ' ';
        }
    }
    return builder;
}

TString TText::JoinWithWhitespaces() const {
    return JoinWithWhitespaces(0, Words.size());
}

TString TText::JoinWithWhitespaces(ui32 l) const {
    return JoinWithWhitespaces(l, Words.size());
}

void TText::Add(const TString& s) {
    Words.push_back(s);
}

void TText::Reverse() {
    std::reverse(Words.begin(), Words.end());
}

TText TText::SubText(ui32 l) const {
    return SubText(l, Size());
}

TText TText::SubText(ui32 l, ui32 r) const {
    ui32 l1 = LetterOnsets.empty() ? 0 : LetterOnsets.size() - JoinWithWhitespaces(l).length();
    ui32 r1 = LetterOnsets.empty() ? 0 : LetterOnsets.size() - JoinWithWhitespaces(r).length();
    return TText(
        TVector<TString>(Words.begin() + l, Words.begin() + r),
        GlobalOffset,
        {},
        TVector<ui32>(
            Onsets.begin() + std::min(l, (ui32)Onsets.size()),
            Onsets.begin() + std::min(r, (ui32)Onsets.size())),
        TVector<ui32>(
            LetterOnsets.begin() + l1,
            LetterOnsets.begin() + r1));
}

std::pair<ui32, ui32> TText::Bounds(ui32 index) const {
    Y_UNUSED(index);
    return {0, Words.size()};
    /*if (Splits.empty()) {
        return {0, Words.size()};
    }
    if (Splits.size() > index) {
        if (Splits[index] > 0) {
            if (index > 0) {
                return {std::max(Splits[index] - 1, Splits[index - 1]), Splits[index]};
            }
            return {Splits[index] - 1, Splits[index]};
        }
        return {Splits[index], Splits[index]};
    }
    return {Words.size(), Words.size()};*/
}

IOutputStream& operator<<(IOutputStream& stream, const TText& text) {
    for (auto& s : text.Words) {
        stream << s << " ";
    }
    return stream;
}

ui32 TText::GetOnset(ui32 index) const {
    if (Onsets.empty()) {
        return GlobalOffset;
    }
    /*if (index >= Onsets.size()) {
        return GlobalOffset + Onsets.back();
    }*/
    return GlobalOffset + Onsets[index];
}

ui32 TText::GetLetterOnset(ui32 index) const {
    if (LetterOnsets.empty()) {
        return GlobalOffset;
    }
    /*if (index >= Onsets.size()) {
        return GlobalOffset + Onsets.back();
    }*/
    return GlobalOffset + LetterOnsets[index];
}

TLikelihoodValue TLikelihoodValue::operator+(const TLikelihoodValue& other) const {
    return TLikelihoodValue(Value + other.Value, Likelihood + other.Likelihood, Corresponding + other.Corresponding);
}

TLikelihoodValue TLikelihoodValue::operator-(const TLikelihoodValue& other) const {
    return TLikelihoodValue(Value - other.Value, Likelihood - other.Likelihood);
}

bool TLikelihoodValue::operator<(const TLikelihoodValue& other) const {
    return (Value == other.Value) ? (Likelihood < other.Likelihood) : (Value < other.Value);
    //const double weight = 1;
    //return (Value + Likelihood * weight) < (other.Value + other.Likelihood * weight);
}

bool TLikelihoodValue::operator<=(const TLikelihoodValue& other) const {
    return !operator>(other);
}

bool TLikelihoodValue::operator>(const TLikelihoodValue& other) const {
    return other.operator<(*this);
}

bool TLikelihoodValue::operator>=(const TLikelihoodValue& other) const {
    return !operator<(other);
}

IOutputStream& operator<<(IOutputStream& stream, const TLikelihoodValue& likelihoodValue) {
    stream << "(" << likelihoodValue.Value << ", " << likelihoodValue.Likelihood << ")";
    return stream;
}

TLikelihoodValue TLikelihoodValue::operator+=(const TLikelihoodValue& other) {
    Value += other.Value;
    Likelihood += other.Likelihood;
    return *this;
}
