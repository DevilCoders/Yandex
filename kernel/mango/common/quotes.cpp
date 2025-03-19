#include "quotes.h"

#include <util/charset/wide.h>
#include <util/generic/map.h>
#include <util/random/mersenne.h>
#include <util/string/cast.h>

#include <queue>

namespace NMango
{

bool TQuotesComparer::operator()(const NMango::TIndexQuote* a, const NMango::TIndexQuote* b) const {
    // force annotation to be topmost
    bool aIsAnn = a->GetData().GetIsAnnotation();
    bool bIsAnn = b->GetData().GetIsAnnotation();
    if (aIsAnn != bIsAnn)
        return aIsAnn && !bIsAnn;
    const TAuthorSearchAttributes &x = a->GetAuthorSearchAttributes();
    const TAuthorSearchAttributes &y = b->GetAuthorSearchAttributes();
    if (x.GetAuthority() == y.GetAuthority()) {
        if (x.GetId() == y.GetId()) {
            return a->GetData().GetSourceUrl() < b->GetData().GetSourceUrl();
        }
        return x.GetId() < y.GetId();
    }
    Y_ASSERT(x.GetId() != y.GetId());
    return x.GetAuthority() > y.GetAuthority();
}

bool TQuotesComparer::operator()(const NMango::TIndexQuote& a, const NMango::TIndexQuote& b) const {
    return operator()(&a, &b);
}

bool TQuotesFilterer::FilterHardcore(TVector<const TIndexQuote*>& quotes) const
{
    size_t numAnnotations = 0;
    ui64 minTime = Max<ui64>();
    for (size_t i = 0; i < quotes.size(); ++i) {
        if (quotes[i]->GetData().GetIsAnnotation())
            ++numAnnotations;
        minTime = Min<ui64>(minTime, quotes[i]->GetData().GetCreationTime());
    }
    // if there are not enough quotes  don't index them
    if (quotes.size() - numAnnotations <= 1)
        return false;

    // remove too fresh resources from regular index
    if (!FastIndex && minTime != Max<ui64>() && TimeNow - TInstant::Seconds(minTime) <= TDuration::Days(3))
        return false;

    return true;
}

void TQuotesFilterer::SortAndFilter(const TIndexQuotes &source, TVector<const TIndexQuote*>& dest) const
{
    const size_t topQuotes = Min(static_cast<size_t>(100), MaxQuotes);
    TMersenne<ui32> rnd;

    TVector<const TIndexQuote*> tmpQuotes;
    for (size_t i = 0; i < source.QuotesSize(); ++i)
        tmpQuotes.push_back(&source.GetQuotes(i));
    Sort(tmpQuotes.begin(), tmpQuotes.end(), TQuotesComparer());

    dest.clear();
    if (Hardcore && !FilterHardcore(tmpQuotes))
        return;

    size_t numQuotes = Min(tmpQuotes.size(), MaxQuotes * 2 - topQuotes);
    float K = numQuotes > topQuotes ? Min(0.5f, (MaxQuotes - topQuotes) * 2 / static_cast<float>(numQuotes - topQuotes) - 1.0f) : 1.0f;

    for (size_t i = 0; i < numQuotes && dest.size() < MaxQuotes; ++i) {
        float level = ((i - topQuotes) * K + (numQuotes - i)) / static_cast<float>(numQuotes - topQuotes);
        if (i < topQuotes || rnd.GenRandReal2() < level) {
            dest.push_back(tmpQuotes[i]);
        }
    }
}

void TQuotesFilterer::SortAndFilter(TIndexQuotes &quotes) const
{
    NMango::TIndexQuotes copyQuotes = quotes;
    TVector<const NMango::TIndexQuote*> filteredQuotes;
    SortAndFilter(copyQuotes, filteredQuotes);
    quotes.ClearQuotes();
    for (TVector<const NMango::TIndexQuote*>::iterator it = filteredQuotes.begin(); it != filteredQuotes.end(); ++it) {
        *quotes.AddQuotes() = **it;
    }
}

bool IsRepost(NMango::TQuoteExtractionType type)
{
    return (type == RETWEET || type == BLOG_FROM_REPOST || type == BLOG_ORIGINAL_FROM_REPOST);
}

bool QuoteHasNoText(NMango::TQuoteExtractionType type) {
    // Technically, BLOG_ORIGINAL_FROM_REPOST quotes have text, but we don't seem to use it anywhere.
    return type == BLOG_ORIGINAL_FROM_REPOST || type == WEB_LIKE || type == WEB_UNLIKE;
}

void FixHtmlAuthority(TIndexQuotes& indexRecord)
{
    float totalAuthoritySum(0.0);
    int weightedQuotesCount(0);

    std::priority_queue<float, TVector<float>, std::greater<float> > pq;
    for (int quote_id = 0, quote_end = indexRecord.QuotesSize(); quote_id < quote_end; ++quote_id) {
        const NMango::TIndexQuote& quote = indexRecord.quotes(quote_id);

        if (quote.GetAuthorSearchAttributes().GetAuthority() > 0.0f && !quote.GetAuthorSearchAttributes().GetIsSpammer()) {
            pq.push(quote.GetAuthorSearchAttributes().GetAuthority());
            if (pq.size() > 3) {
                pq.pop();
            }
        }

        while (!pq.empty()) {
            totalAuthoritySum += pq.top();
            pq.pop();
            ++weightedQuotesCount;
        }
    }

    // set authority if not set
    if (weightedQuotesCount) {
        float autoAuthority = (totalAuthoritySum / (float)weightedQuotesCount);
        for (int quote_id = 0, quote_end = indexRecord.QuotesSize(); quote_id < quote_end; ++quote_id) {
            NMango::TIndexQuote* quote = indexRecord.MutableQuotes(quote_id);
            if (quote->GetAuthorSearchAttributes().GetAuthority() <= 0.0f
                && !quote->GetAuthorSearchAttributes().GetIsSpammer()
                && quote->GetData().GetIsAnnotation())
            {
                quote->MutableAuthorSearchAttributes()->SetAuthority(autoAuthority);
            }
        }
    }
}
TString GetShortTweetId(const TString& src)
{
    static TString twiNetId = ToString ((int)NMango::TWITTER) + ":";
    TStringBuf rl, id;
    TStringBuf(src).RSplit('/', rl, id);
    Y_VERIFY(rl.find(TStringBuf("twitter")) != TStringBuf::npos, "format changed too much");
    return twiNetId + id;
}
TString GetQuoteDelId(const NMango::TQuoteData& q) {
    if (q.GetExtractionType() == NMango::TWEET || q.GetExtractionType() == NMango::RETWEET)
        return GetShortTweetId(q.GetSourceUrl());
    return TString();
}
TString GetQuoteDelId(const NMango::TQuote& q) {
    return GetQuoteDelId(q.GetData());
}

void TAutogeneratedTextRemover::SetAnnotation(const TUtf16String& annotationText) {
    AnnotationText = annotationText;
    Collapse(AnnotationText);
    Strip(AnnotationText);
}

void TAutogeneratedTextRemover::RemoveFrom(TUtf16String& quoteText) const {
    Collapse(quoteText);
    Strip(quoteText);

    if (AnnotationText.empty() || quoteText.empty()) {
        return;
    }
    size_t lcsStart = 0;
    size_t lcsLength = 0;
    PrevDp.assign(quoteText.size(), 0);
    Dp.assign(quoteText.size(), 0);
    for (size_t i = 0; i < AnnotationText.size(); i++) {
        for (size_t j = 0; j < quoteText.size(); j++) {
            if (AnnotationText[i] == quoteText[j]) {
                size_t newLcsLength = (i == 0 || j == 0) ? 1 : (PrevDp[j - 1] + 1);
                Dp[j] = newLcsLength;
                if (newLcsLength > lcsLength) {
                    lcsStart = j - newLcsLength + 1;
                    lcsLength = newLcsLength;
                }
            } else {
                Dp[j] = 0;
            }
        }
        Dp.swap(PrevDp);
    }

    if (lcsLength > 10) {
        TUtf16String::const_iterator lcsBegin = quoteText.begin() + lcsStart;
        TUtf16String::const_iterator lcsEnd = lcsBegin + lcsLength;
        if (std::count(lcsBegin, lcsEnd, ' ') >= 2) {
            quoteText.erase(lcsBegin, lcsEnd);
        }
    }
}

void CountReposts(NMango::TIndexQuotes& indexQuotes) {
    TMap<TString, NMango::TIndexQuote*> originalQuotes;
    for (size_t i = 0; i < indexQuotes.QuotesSize(); ++i) {
        NMango::TIndexQuote *quote = indexQuotes.MutableQuotes(i);
        quote->MutableData()->SetRepostCount(0);
        NMango::TQuoteExtractionType type = quote->GetData().GetExtractionType();
        if (!IsRepost(type) && quote->GetData().HasSourceUrl()) {
            originalQuotes[quote->GetData().GetSourceUrl()] = quote;
        }
    }

    for (size_t i = 0; i < indexQuotes.QuotesSize(); ++i) {
        const NMango::TIndexQuote& quote = indexQuotes.GetQuotes(i);
        if (IsRepost(quote.GetData().GetExtractionType())) {
            TMap<TString, NMango::TIndexQuote*>::iterator it = originalQuotes.find(quote.GetData().GetOriginalSourceUrl());
            if (it != originalQuotes.end()) {
                it->second->MutableData()->SetRepostCount(it->second->GetData().GetRepostCount() + 1);
                it->second->MutableData()->SetMaxAuthorityAmongReposts(
                        Max(it->second->GetData().GetMaxAuthorityAmongReposts(), quote.GetAuthorSearchAttributes().GetAuthority()));
                it->second->MutableData()->SetSumAuthorityAmongReposts(
                        it->second->GetData().GetSumAuthorityAmongReposts() + quote.GetAuthorSearchAttributes().GetAuthority());
            }
        }
    }
}

} // NMango

