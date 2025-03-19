#pragma once

//
// Created by Mikhail Yutman on 30.03.2020.
//

#include "structures.h"

#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/string/builder.h>
#include <util/string/split.h>

#include <algorithm>

struct TText {
    TVector<TString> Words{};
    ui32 GlobalOffset{};
    TVector<ui32> Splits{};
    TVector<ui32> Onsets{};
    TVector<ui32> LetterOnsets{};

    TText() = default;

    explicit TText(
        TVector<TString> words,
        ui32 globalOffset,
        TVector<ui32> splits = {},
        TVector<ui32> onsets = {},
        TVector<ui32> letterOnsets = {})
        : Words(std::move(words))
        , GlobalOffset(globalOffset)
        , Splits(std::move(splits))
        , Onsets(std::move(onsets))
        , LetterOnsets(std::move(letterOnsets))
    {
    }

    [[nodiscard]] int Size() const;

    [[nodiscard]] TString JoinWithWhitespaces() const;

    [[nodiscard]] TString JoinWithWhitespaces(ui32 l) const;

    [[nodiscard]] TString JoinWithWhitespaces(ui32 l, ui32 r) const;

    void Add(const TString& s);

    void Reverse();

    [[nodiscard]] TText SubText(ui32 l) const;

    [[nodiscard]] TText SubText(ui32 l, ui32 r) const;

    [[nodiscard]] std::pair<ui32, ui32> Bounds(ui32 index) const;

    [[nodiscard]] ui32 GetOnset(ui32 index) const;

    [[nodiscard]] ui32 GetLetterOnset(ui32 index) const;

    friend IOutputStream& operator<<(IOutputStream& stream, const TText& text);
};

struct TRecordBitsMarkups {
    TString RecordId{};
    TString S3Key{};
    TVector<TVector<TText>> BitsMarkups{};

    TRecordBitsMarkups() = default;
    explicit TRecordBitsMarkups(TString id)
        : RecordId(std::move(id))
    {
    }
    explicit TRecordBitsMarkups(TString id, TString s3Key)
        : RecordId(std::move(id))
        , S3Key(std::move(s3Key))
    {
    }
};

struct TUnitedRecordBitsMarkups {
    TString RecordId{};
    TString S3Key{};
    TVector<TText> BitsMarkups{};

    TUnitedRecordBitsMarkups() = default;
    explicit TUnitedRecordBitsMarkups(TString id)
        : RecordId(std::move(id))
    {
    }
    explicit TUnitedRecordBitsMarkups(TString id, TString s3Key)
        : RecordId(std::move(id))
        , S3Key(std::move(s3Key))
    {
    }
};

struct TRecordJoin {
    TString RecordId{};
    TString S3Key{};
    TText JoinedText;

    TRecordJoin() = default;
    explicit TRecordJoin(TString id)
        : RecordId(std::move(id))
    {
    }
    explicit TRecordJoin(TString id, TString s3Key)
        : RecordId(std::move(id))
        , S3Key(std::move(s3Key))
    {
    }
    explicit TRecordJoin(TString id, TText joinedText)
        : RecordId(std::move(id))
        , JoinedText(std::move(joinedText))
    {
    }
    explicit TRecordJoin(TString id, TString s3Key, TText joinedText)
        : RecordId(std::move(id))
        , S3Key(std::move(s3Key))
        , JoinedText(std::move(joinedText))
    {
    }
};

struct TState {
    ui32 PreviousLevelR;
    ui32 CurrentLevelL;

    TState() = default;
    explicit TState(ui32 r, ui32 l)
        : PreviousLevelR(r)
        , CurrentLevelL(l)
    {
    }
};

template <class T>
struct TDpValues {
    TVector<TVector<T>> Values;

    explicit TDpValues(ui32 dim1, ui32 dim2)
        : Values(dim1, TVector<T>(dim2))
    {
    }
};

struct TDpBestStates {
    TVector<TVector<TState>> States;

    explicit TDpBestStates(ui32 dim1, ui32 dim2)
        : States(dim1, TVector<TState>(dim2))
    {
    }
};

struct TLikelihoodValue {
    int Value;
    double Likelihood;
    ui32 Corresponding;

    TLikelihoodValue(int value = 0, double likelihood = 0, ui32 corresponding = 0)
        : Value(value)
        , Likelihood(likelihood)
        , Corresponding(corresponding)
    {
    }

    [[nodiscard]] TLikelihoodValue operator+(const TLikelihoodValue& other) const;

    TLikelihoodValue operator+=(const TLikelihoodValue& other);

    [[nodiscard]] TLikelihoodValue operator-(const TLikelihoodValue& other) const;

    [[nodiscard]] bool operator<(const TLikelihoodValue& other) const;

    [[nodiscard]] bool operator<=(const TLikelihoodValue& other) const;

    [[nodiscard]] bool operator>(const TLikelihoodValue& other) const;

    [[nodiscard]] bool operator>=(const TLikelihoodValue& other) const;

    friend IOutputStream& operator<<(IOutputStream& stream, const TLikelihoodValue& likelihoodValue);
};
