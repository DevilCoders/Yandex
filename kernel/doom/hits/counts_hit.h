#pragma once

#include <util/system/yassert.h>
#include <util/stream/output.h>

#include <library/cpp/wordpos/wordpos.h>

#include <search/panther/custom/images_v0_1/stream_factory_images_v0_1.h>

namespace NDoom {

/**
 * Hit in interim index.
 *
 * Internal format, matches the one in TWordPosition.
 * [doc:26][count:15][streamType:6][:2][:4]
 *
 * Compare that to default index format:
 * [doc:26][break:15][word:6][relev:2][form:4]
 */
class TCountsHit {
public:
    enum {
        MinValue = 1,
        MaxValue = BREAK_LEVEL_Max
    };

    TCountsHit(ui32 docId, NDoom::EStreamType streamType, ui32 value) {
        Init(docId, streamType, value);
    }

    TCountsHit(): Data_(0) {}

    enum {
        FormBits = 0
    };

    bool IsNull() const {
        return Data_ == 0; /* Value should not be 0 in a valid hit, so this check is correct. */
    }

    ui32 DocId() const {
        return Data_ >> DOC_LEVEL_Shift;
    }

    void SetDocId(const ui32 docId) {
        SetDocId(Data_, docId);
    }

    NDoom::EStreamType StreamType() const {
        return static_cast<NDoom::EStreamType>(TWordPosition::Word(Data_));
    }

    ui32 Value() const {
        return TWordPosition::Break(Data_);
    }

    void SetValue(ui32 value) {
        TWordPosition::SetBreak(Data_, value);

        Y_ASSERT(Value() == value);
    }

    SUPERLONG ToSuperLong() const {
        return Data_;
    }

    SUPERLONG Id() const {
        TCountsHit temp = *this;
        temp.SetValue(0);
        return temp.ToSuperLong();
    }

    static TCountsHit FromSuperLong(const SUPERLONG& superLong) {
        TCountsHit result;
        result.Data_ = superLong;
        return result;
    }

    friend bool operator<(const TCountsHit& l, const TCountsHit& r) {
        return l.Data_ < r.Data_;
    }

    friend bool operator==(const TCountsHit& l, const TCountsHit& r) {
        return l.Data_ == r.Data_;
    }

    friend bool operator>=(const TCountsHit& l, const TCountsHit& r) {
        return l.Data_ >= r.Data_;
    }

    friend IOutputStream& operator<<(IOutputStream& stream, const TCountsHit& hit) {
        stream <<
            "[" << hit.DocId() <<
            "." << static_cast<ui32>(hit.StreamType()) <<
            "." << hit.Value() <<
            "]";
        return stream;
    }

private:
    void Init(ui32 docId, ui32 streamType, ui32 value) {
        Y_ASSERT(value >= MinValue && value <= MaxValue);

        Data_ = 0;
        TWordPosition::SetBreak(Data_, value);
        TWordPosition::SetWord(Data_, streamType);
        SetDocId(Data_, docId);

        Y_ASSERT(DocId() == docId);
        Y_ASSERT(StreamType() == static_cast<NDoom::EStreamType>(streamType));
        Y_ASSERT(Value() == value);
    }

    static void SetDocId(SUPERLONG& hitData, ui32 docId) {
        hitData &= (SUPERLONG(1) << DOC_LEVEL_Shift) - 1;
        hitData |= SUPERLONG(docId) << DOC_LEVEL_Shift;
    }

private:
    SUPERLONG Data_;
};


struct TInterimHitIdLess {
    bool operator()(const TCountsHit& l, const TCountsHit& r) const {
        return l.Id() < r.Id();
    }
};


struct TInterimHitDocIdLess {
    bool operator()(const TCountsHit& l, const TCountsHit& r) const {
        return l.DocId() < r.DocId();
    }

    bool operator()(const TCountsHit& l, ui32 r) const {
        return l.DocId() < r;
    }

    bool operator()(ui32 l, const TCountsHit& r) const {
        return l < r.DocId();
    }
};


} // namespace NDoom
