#pragma once

#include <kernel/snippets/schemaorg/question/question.h>
#include <kernel/snippets/span/span.h>

#include <util/generic/hash.h>

namespace NSnippets
{
    class TSentsMatchInfo;
    class TSchemaOrgArchiveViewer;

    struct TAnswerMatchValues
    {
        int MaxCommonSubstrWordCount = 0;
        int FirstWordOfMaxCSOfsInSnip = -1;
        int FirstWordOfMaxCSOfsInAnswer = -1;
        int SpanLen = 0;
    };

    struct TAnswerMatchFactors {
        float WordCount;
        float UpvoteCount;
        float MaxSpanMatchRatioInSnip;
        float MaxSpanMatchRatioInAnswer;
        float FirstMatchPosRatioInSnip;
        float FirstMatchPosRatioInAnswer;
    };

    class TAnswerInfo {
    private:
        const NSchemaOrg::TAnswer* Answer = nullptr;
        TSpan WordSpan;
        THashMap<size_t, TVector<size_t>> WordHash2Pos;

    public:
        TAnswerInfo(const NSchemaOrg::TAnswer* answer);
        TAnswerMatchFactors CalcMatchFactors(const TSentsMatchInfo& snip, const TSpans& wSpans) const;

    private:
        TAnswerMatchValues CalcMatchValues(const TSentsMatchInfo& snip, const TSpans& wSpans) const;
    };

    class TSchemaOrgQuestionInfo {
    public:
        const TSchemaOrgArchiveViewer* SchemaOrgViewer = nullptr;
        const NSchemaOrg::TQuestion* Question = nullptr;
        THolder<TAnswerInfo> BestAnswer;
        TVector<TAnswerInfo> Answers;

    private:
        NSchemaOrg::TAnswer BestAnswerData;

    public:
        TSchemaOrgQuestionInfo(const TSchemaOrgArchiveViewer* schemaOrgViewer);
    };
}

