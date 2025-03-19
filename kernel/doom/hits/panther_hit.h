#pragma once

#include <util/generic/utility.h> /* For ClampVal. */
#include <util/stream/output.h>

#include <library/cpp/wordpos/wordpos.h>


namespace NDoom {

/**
 * Hit in panther index.
 *
 * Internal format:
 * [doc:32][relevance:32]
 */
class TPantherHit {
public:
    TPantherHit() = default;

    TPantherHit(ui32 docId, ui32 relevance)
        : DocId_(docId)
        , Relevance_(relevance)
    {
    }

    ui32 DocId() const {
        return DocId_;
    }

    void SetDocId(ui32 docId) {
        DocId_ = docId;
    }

    ui32 Relevance() const {
        return Relevance_;
    }

    void SetRelevance(ui32 relevance) {
        Relevance_ = relevance;
    }

    SUPERLONG ToSuperLong() const {
        SUPERLONG superLong = 0;
        TWordPosition::SetDoc(superLong, DocId_);
        TWordPosition::SetBreak(superLong, Relevance_);
        return superLong;
    }

    static TPantherHit FromSuperLong(const SUPERLONG& superLong) {
        return { TWordPosition::Doc(superLong), TWordPosition::Break(superLong) };
    }

    static ui32 IntRelevance(double relevance) {
        relevance = ClampVal(relevance, 0.0, 1.0);
        return static_cast<ui32>(relevance * 32767.0);
    }

    static double FloatRelevance(ui32 relevance) {
        return relevance / 32767.0;
    }

    enum {
        FormBits = 0
    };

    friend bool operator<(const TPantherHit& l, const TPantherHit& r) {
        if (l.DocId_ != r.DocId_) {
            return l.DocId_ < r.DocId_;
        }
        return l.Relevance_ < r.Relevance_;
    }

    friend bool operator>=(const TPantherHit& l, const TPantherHit& r) {
        if (l.DocId_ != r.DocId_) {
            return l.DocId_ > r.DocId_;
        }
        return l.Relevance_ >= r.Relevance_;
    }

    friend bool operator==(const TPantherHit& l, const TPantherHit& r) {
        return l.DocId_ == r.DocId_ && l.Relevance_ == r.Relevance_;
    }


    friend IOutputStream& operator<<(IOutputStream& stream, const TPantherHit& hit) {
        stream << "[" << hit.DocId() << "." << hit.Relevance() << "]";
        return stream;
    }

private:
    ui32 DocId_ = 0;
    ui32 Relevance_ = 0;
};


struct TPantherHitRelevanceLess {
    bool operator()(const TPantherHit &l, const TPantherHit &r) {
        return l.Relevance() < r.Relevance();
    }

    bool operator()(const SUPERLONG &l, const SUPERLONG &r) {
        return TPantherHit::FromSuperLong(l).Relevance() < TPantherHit::FromSuperLong(r).Relevance();
    }
};

struct TPantherHitRelevanceMore {
    bool operator()(const TPantherHit &l, const TPantherHit &r) {
        return l.Relevance() > r.Relevance();
    }

    bool operator()(const SUPERLONG &l, const SUPERLONG &r) {
        return TPantherHit::FromSuperLong(l).Relevance() > TPantherHit::FromSuperLong(r).Relevance();
    }
};

} // namespace NDoom
