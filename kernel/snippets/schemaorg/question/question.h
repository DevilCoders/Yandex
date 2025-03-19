#pragma once

#include <util/generic/list.h>
#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/algorithm.h>

namespace NSchemaOrg {
    class TAnswer {
    public:
        TUtf16String Text;
        TUtf16String AuthorName;
        TMaybe<int> UpvoteCount;
        bool IsAccepted = false;

        bool operator<(const TAnswer& another) const;
        void SetUpvoteCount(const TStringBuf& source);
    };
    class TQuestion {
    public:
        TUtf16String QuestionText;
        TUtf16String AuthorName;
        TUtf16String DatePublished;
        TUtf16String AnswerCount;

        TAnswer AcceptedAnswer;
        TList<TAnswer> SuggestedAnswer;
        TUtf16String BestAnswerLabelTextNode;

    public:
        TAnswer GetBestAnswer() const;
        void GetAllNotEmptyAnswers(TVector<const TAnswer*>& target) const;
        bool HasApprovedAnswer() const;
        void CleanAnswer(TUtf16String& answer) const;
    };

} // namespace NSchemaOrg
