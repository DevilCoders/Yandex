#pragma once

#include <kernel/doom/hits/superlong_hit.h>

#include <util/generic/fwd.h>
#include <util/stream/output.h>

#include <library/cpp/wordpos/wordpos.h>

namespace NDoom {


class TAttributesHit {
public:
    TAttributesHit() = default;

    TAttributesHit(ui32 docId)
        : DocId_(docId)
    {

    }

    TAttributesHit(TSuperlongHit hit)
        : DocId_(hit.DocId())
    {
    }

    ui32 DocId() const {
        return DocId_;
    }

    void SetDocId(ui32 docId) {
        DocId_ = docId;
    }

    friend bool operator!=(const TAttributesHit& l, const TAttributesHit& r) {
       return l.DocId_ != r.DocId_;
    }

    friend bool operator==(const TAttributesHit& l, const TAttributesHit& r) {
        return l.DocId_ == r.DocId_;
    }

    friend bool operator<(const TAttributesHit& l, const TAttributesHit& r) {
        return l.DocId_ < r.DocId_;
    }

    friend bool operator>=(const TAttributesHit& l, const TAttributesHit& r) {
        return l.DocId_ >= r.DocId_;
    }

    friend IOutputStream& operator<<(IOutputStream& stream, const TAttributesHit& hit) {
        stream << "[" << hit.DocId() << "]";
        return stream;
    }

    static TAttributesHit FromSuperLong(const SUPERLONG& superlong) {
        return { TWordPosition::Doc(superlong) };
    }

private:
    ui32 DocId_ = 0;
};


} // namespace NDoom
