#pragma once

#include <util/stream/output.h>

#include "wordpos.h"

/**
 * Wrapper for a default SUPERLONG hit.
 *
 * Internal format:
 * [doc:26][break:15][word:6][relev:2][form:4]
 */
class TSuperlongHit {
public:
    enum {
        FormBits = NFORM_LEVEL_Bits
    };

    TSuperlongHit() = default;

    TSuperlongHit(SUPERLONG data)
        : Data_(data)
    {
    }

    TSuperlongHit(ui32 docId, ui32 breuk, ui32 word, ui32 relevance, ui32 form)
        : Data_(0)
    {
        SetDocId(docId);
        SetBreak(breuk);
        SetWord(word);
        SetRelevance(relevance);
        SetForm(form);

        Y_ASSERT(docId == DocId());
        Y_ASSERT(breuk == Break());
        Y_ASSERT(word == Word());
        Y_ASSERT(relevance == Relevance());
        Y_ASSERT(form == Form());
    }

    ui32 DocId() const {
        return TWordPosition::Doc(Data_);
    }

    void SetDocId(ui32 docId) {
        TWordPosition::SetDoc(Data_, docId);
    }

    ui32 Break() const {
        return TWordPosition::Break(Data_);
    }

    void SetBreak(ui32 breuk) {
        TWordPosition::SetBreak(Data_, breuk);
    }

    ui32 Word() const {
        return TWordPosition::Word(Data_);
    }

    void SetWord(ui32 word) {
        TWordPosition::SetWord(Data_, word);
    }

    ui32 Relevance() const {
        return TWordPosition::GetRelevLevel(Data_);
    }

    void SetRelevance(ui32 relevance) {
        TWordPosition::SetRelevLevel(Data_, relevance);
    }

    ui32 Form() const {
        return TWordPosition::Form(Data_);
    }

    void SetForm(ui32 form) {
        TWordPosition::SetWordForm(Data_, form);
    }

    SUPERLONG ToSuperLong() const {
        return Data_;
    }

    static TSuperlongHit FromSuperLong(const SUPERLONG& superLong) {
        return { superLong };
    }

    friend bool operator<(const TSuperlongHit& l, const TSuperlongHit& r) {
        return l.Data_ < r.Data_;
    }

    friend bool operator>=(const TSuperlongHit& l, const TSuperlongHit& r) {
        return l.Data_ >= r.Data_;
    }

    friend bool operator==(const TSuperlongHit& l, const TSuperlongHit& r) {
        return l.Data_ == r.Data_;
    }

    friend bool operator!=(const TSuperlongHit& l, const TSuperlongHit& r) {
        return l.Data_ != r.Data_;
    }

    friend IOutputStream& operator<<(IOutputStream& stream, const TSuperlongHit& hit) {
        stream << "[" << hit.DocId() << "." << hit.Break() << "." << hit.Word() << "." << hit.Relevance() << "." << hit.Form() << "]";
        return stream;
    }

private:
    SUPERLONG Data_ = 0;
};
