#pragma once

#include "superlong_hit.h"

#include <util/generic/bitops.h>
#include <util/stream/output.h>
#include <util/system/yassert.h>

#include <utility>

namespace NDoom {

/**
 * Wrapper for a ReqBundle hit (aka TPosition).
 * See SEARCH-2607
 * Diff from a default SUPERLONG hit: form:4 -> form:12, doc:26 -> doc:29
 *
 * Internal format:
 * [range:13][break:15][word:6][relev:2][form:12]
 * For doc separation see SEARCH-7791
 */
class TReqBundleHit {
public:
    enum {
        FormOffset = 0,
        FormBits = 12,
        RelevOffset = FormOffset + FormBits,
        RelevBits = 2,
        WordOffset = RelevOffset + RelevBits,
        WordBits = 6,
        BreakOffset = WordOffset + WordBits,
        BreakBits = 15,
        RangeOffset = BreakOffset + BreakBits,
        RangeBits = 13,
        TotalBits = RangeOffset + RangeBits,
    };

    static_assert(TotalBits <= 64, "TReqBundleHit must fit in ui64");

    TReqBundleHit()
        : Data_(0)
        , DocId_(0)
    {
    }

    TReqBundleHit(ui32 docId, ui32 breuk, ui32 word, ui32 relevance, ui32 form, ui32 range)
        : Data_((static_cast<ui64>(breuk) << BreakOffset)
                | (static_cast<ui64>(word) << WordOffset)
                | (static_cast<ui64>(relevance) << RelevOffset)
                | (static_cast<ui64>(range) << RangeOffset)
                | static_cast<ui64>(form))
        , DocId_(docId)
    {
        Y_ASSERT(docId == DocId());
        Y_ASSERT(breuk == Break());
        Y_ASSERT(word == Word());
        Y_ASSERT(relevance == Relevance());
        Y_ASSERT(form == Form());
        Y_ASSERT(range == Range());
    }

    TReqBundleHit(const TSuperlongHit& hit)
        : TReqBundleHit(hit.DocId(), hit.Break(), hit.Word(), hit.Relevance(), hit.Form(), 0)
    {
    }

    static TReqBundleHit FromSuperLong(const SUPERLONG& superLong) {
        return TReqBundleHit(TSuperlongHit::FromSuperLong(superLong));
    }

    ui32 DocId() const {
        return DocId_;
    }

    void SetDocId(ui32 docId) {
        DocId_ = docId;
    }

    ui32 Break() const {
        return SelectBits<BreakOffset, BreakBits, ui64>(Data_);
    }

    void SetBreak(ui32 breuk) {
        SetBits<BreakOffset, BreakBits, ui64>(Data_, breuk);
    }

    ui32 Word() const {
        return SelectBits<WordOffset, WordBits, ui64>(Data_);
    }

    void SetWord(ui32 word) {
        SetBits<WordOffset, WordBits, ui64>(Data_, word);
    }

    ui32 Relevance() const {
        return SelectBits<RelevOffset, RelevBits, ui64>(Data_);
    }

    void SetRelevance(ui32 relevance) {
        SetBits<RelevOffset, RelevBits, ui64>(Data_, relevance);
    }

    ui32 Form() const {
        return SelectBits<FormOffset, FormBits, ui64>(Data_);
    }

    void SetForm(ui32 form) {
        SetBits<FormOffset, FormBits, ui64>(Data_, form);
    }

    ui32 Range() const {
        return SelectBits<RangeOffset, RangeBits, ui64>(Data_);
    }

    void SetRange(ui32 range) {
        SetBits<RangeOffset, RangeBits, ui64>(Data_, range);
    }

    friend bool operator<(const TReqBundleHit& l, const TReqBundleHit& r) {
        return std::pair<ui32, ui64>(l.DocId_, l.Data_) < std::pair<ui32, ui64>(r.DocId_, r.Data_);
    }

    friend bool operator>=(const TReqBundleHit& l, const TReqBundleHit& r) {
        return std::pair<ui32, ui64>(l.DocId_, l.Data_) >= std::pair<ui32, ui64>(r.DocId_, r.Data_);
    }

    friend bool operator==(const TReqBundleHit& l, const TReqBundleHit& r) {
        return std::pair<ui32, ui64>(l.DocId_, l.Data_) == std::pair<ui32, ui64>(r.DocId_, r.Data_);
    }

    friend bool operator!=(const TReqBundleHit& l, const TReqBundleHit& r) {
        return std::pair<ui32, ui64>(l.DocId_, l.Data_) != std::pair<ui32, ui64>(r.DocId_, r.Data_);
    }

    friend IOutputStream& operator<<(IOutputStream& stream, const TReqBundleHit& hit) {
        stream << "[" << hit.DocId() << "." << hit.Range() << "." << hit.Break() << "." << hit.Word() << "." << hit.Relevance() << "." << hit.Form() << "]";
        return stream;
    }

private:
    ui64 Data_ = 0;
    ui32 DocId_ = 0;
};

} // namespace NDoom
