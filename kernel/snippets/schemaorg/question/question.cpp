#include "question.h"

#include <util/charset/wide.h>
#include <util/string/cast.h>
#include <util/string/strip.h>
#include <util/string/subst.h>
#include <util/str_stl.h>

namespace NSchemaOrg {
    static const TUtf16String BEST_ANSWER = u"Лучший ответ";
    static const TUtf16String CUT_LINK = u"[ссылка появится после проверки модератором]";
    static const TUtf16String SPACE = TUtf16String(wchar16(' '));

    bool TAnswer::operator<(const TAnswer& another) const {
        if (IsAccepted) {
            return false;
        }
        if (another.IsAccepted) {
            return true;
        }
        return UpvoteCount.Defined() && another.UpvoteCount.Defined() &&
            *UpvoteCount < *another.UpvoteCount;
    }

    void TAnswer::SetUpvoteCount(const TStringBuf& source) {
        int count;
        if (TryFromString(StripString(source), count)) {
            UpvoteCount = count;
        }
    }

    TAnswer TQuestion::GetBestAnswer() const {
        if (AcceptedAnswer.Text) {
            return AcceptedAnswer;
        }
        if (SuggestedAnswer) {
            if (BestAnswerLabelTextNode == BEST_ANSWER) {
                return SuggestedAnswer.front();
            }
            int bestUpvote = -1;
            const TAnswer* best = nullptr;
            for (const TAnswer& sa : SuggestedAnswer) {
                if (sa.UpvoteCount.Defined() && *sa.UpvoteCount > bestUpvote) {
                    best = &sa;
                    bestUpvote = *sa.UpvoteCount;
                }
            }
            if (best) {
                return *best;
            }
            return SuggestedAnswer.front();
        }
        return TAnswer();
    }

    void TQuestion::GetAllNotEmptyAnswers(TVector<const TAnswer*>& target) const {
        target.reserve(SuggestedAnswer.size() + 1);
        if (!AcceptedAnswer.Text.empty()) {
            target.push_back(&AcceptedAnswer);
        }
        for (const auto& ans : SuggestedAnswer) {
            if (!ans.Text.empty()) {
                target.push_back(&ans);
            }
        }
        Sort(target.rbegin(), target.rend(),
            [](const TAnswer* left, const TAnswer* right) { return *left < *right; });
    }

    bool TQuestion::HasApprovedAnswer() const {
        return !AcceptedAnswer.Text.empty() || BestAnswerLabelTextNode == BEST_ANSWER;
    }

    void TQuestion::CleanAnswer(TUtf16String& answer) const {
        SubstGlobal(answer, CUT_LINK, SPACE);
        Strip(answer);
        Collapse(answer);
    }
}
