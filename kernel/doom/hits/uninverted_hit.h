#pragma once

#include <util/generic/bitops.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>

#include <kernel/doom/enums/namespace.h>
#include <kernel/doom/enums/stream_type.h>

namespace NDoom {

/**
 * Hit in uninverted index.
 *
 * Internal format:
 * [break:15][word:6][stream:6][term:30] + [form:32]
 *
 * Compare that to default index format:
 * [doc:26][break:15][word:6][relev:2][form:4]
 */
class TUninvertedHit {
public:
    static_assert(BreakSize + WordSize + StreamTypeSize + TermIdSize <= 64, "TUninvertedHit fields should fit in ui64.");

    enum EOffset {
        TermIdOffset = 0,
        StreamTypeOffset = TermIdOffset + TermIdSize,
        WordOffset = StreamTypeOffset + StreamTypeSize,
        BreakOffset = WordOffset + WordSize,
    };

    TUninvertedHit(NDoom::EStreamType streamType, ui32 breuk, ui32 word, ui32 termId, ui32 form) {
        Data_ =
            (static_cast<ui64>(termId) << TermIdOffset) |
            (static_cast<ui64>(streamType) << StreamTypeOffset) |
            (static_cast<ui64>(word) << WordOffset) |
            (static_cast<ui64>(breuk) << BreakOffset);

        Form_ = form;

        Y_ENSURE(form == Form());
        Y_ENSURE(termId == TermId());
        Y_ENSURE(streamType == StreamType());
        Y_ENSURE(word == Word());
        Y_ENSURE(breuk == Break());
    }

    template<class GenericHit>
    TUninvertedHit(const GenericHit& hit, NDoom::EStreamType streamType, ui32 termId)
        : TUninvertedHit(streamType, hit.Break(), hit.Word(), termId, hit.Form())
    {

    }

    TUninvertedHit()
        : Data_(0)
        , Form_(0)
    {

    }

    ui32 Break() const {
        return SelectBits<BreakOffset, BreakSize, ui64>(Data_);
    }

    ui32 Word() const {
        return SelectBits<WordOffset, WordSize, ui64>(Data_);
    }

    NDoom::EStreamType StreamType() const {
        return static_cast<NDoom::EStreamType>(SelectBits<StreamTypeOffset, StreamTypeSize, ui64>(Data_));
    }

    ui32 TermId() const {
        return SelectBits<TermIdOffset, TermIdSize, ui64>(Data_);
    }

    ui32 Form() const {
        return Form_;
    }

    ui64 Position() const {
        return SelectBits<StreamTypeOffset, StreamTypeSize + WordSize + BreakSize, ui64>(Data_);
    }

    ui64 BreakAndWord() const {
        return SelectBits<WordOffset, WordSize + BreakSize, ui64>(Data_);
    }

    friend bool operator<(const TUninvertedHit &l, const TUninvertedHit &r) {
        if (l.Data_ != r.Data_) {
            return l.Data_ < r.Data_;
        }
        return l.Form_ < r.Form_;
    }

    friend bool operator==(const TUninvertedHit &l, const TUninvertedHit &r) {
        return l.Data_ == r.Data_ && l.Form_ == r.Form_;
    }


    friend IOutputStream& operator<<(IOutputStream& stream, const TUninvertedHit& hit) {
        stream << "[" << hit.Break() << "." << hit.Word() << "." << int(hit.StreamType()) << "." << hit.TermId() << "." << hit.Form() << "]";
        return stream;
    }

private:
    ui64 Data_ = 0;
    ui32 Form_ = 0;
};


} // namespace NDoom
