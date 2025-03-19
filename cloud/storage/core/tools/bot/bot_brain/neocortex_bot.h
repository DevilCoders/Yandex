#pragma once

#include <ml/neocortex/neocortex_lib/classifiers.h>

#include <library/cpp/json/json_value.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NCoolBot {

////////////////////////////////////////////////////////////////////////////////

class TNeocortexBot: public TAtomicRefCount<TNeocortexBot>
{
public:
    TNeocortexBot(const NJson::TJsonValue& config);

public:
    struct TResult
    {
        TString Text;
        float Score = -Max<float>();
    };

    TResult SelectAnswer(const TString& message);

private:
    NNeocortex::TTextClassifierPack Pack;
    TVector<TUtf16String> RareWords;

    struct TAnswer
    {
        TString Text;
        NNeocortex::ITextClassifier::TContext Context;
    };
    TVector<TAtomicSharedPtr<TAnswer>> Answers;

    struct TSettings
    {
        double DropAnswerProbability = 0.5;
        double MinAnswerScore = 2;
    } Settings;
};

using TBotPtr = TIntrusivePtr<TNeocortexBot>;

}   // namespace NCoolBot
