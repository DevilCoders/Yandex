#pragma once

#include "decoded_form_ref.h"
#include "key_class.h"

#include <kernel/search_types/search_types.h>

#include <library/cpp/charset/codepage.h>
#include <kernel/keyinv/invkeypos/keychars.h>
#include <library/cpp/langs/langs.h>

#include <util/system/yassert.h>
#include <util/generic/strbuf.h>
#include <util/generic/flags.h>
#include <util/generic/vector.h>

namespace NDoom {


class TDecodedKey {
public:
    enum {
        MaxFormsCount = (ui32(1) << 12),
        MaxFormsBufferSize = Max<ui16>()
    };

public:
    TStringBuf Lemma() const {
        return { LemmaBuffer_.data(), LemmaBuffer_.size() };
    }

    EKeyClass Class() const {
        return ClassifyKey(Lemma());
    }

    i32 FormIndex(const TDecodedFormRef& form, i32 defaultValue = -1) const {
        for (size_t i = 0; i < FormCount(); ++i) {
            if (Form(i) == form) {
                return i;
            }
        }
        return defaultValue;
    }

    void SetLemma(const TStringBuf& lemma) {
        LemmaBuffer_.assign(lemma);
    }

    size_t FormCount() const {
        return FormStartPositions_.size();
    }

    TDecodedFormRef Form(size_t index) const {
        ui16 start = FormStartPositions_[index];
        ui16 end = (index + 1 < FormStartPositions_.size() ? FormStartPositions_[index + 1] : FormsBuffer_.size());
        Y_ASSERT(start <= end && end <= FormsBuffer_.size());
        return { TStringBuf(FormsBuffer_.data() + start, FormsBuffer_.data() + end) };
    }

    void AddForm(const TDecodedFormRef& form) {
        Y_ENSURE(TryAddForm(form));
    }

    [[nodiscard]]
    bool TryAddForm(const TDecodedFormRef& form) {
        return TryAddForm(form.Language(), form.Flags(), form.Text());
    }

    void AddForm(ELanguage language, EFormFlags flags, const TStringBuf& text) {
        Y_ENSURE(TryAddForm(language, flags, text));
    }

    [[nodiscard]]
    bool TryAddForm(ELanguage language, EFormFlags flags, const TStringBuf& text) {
        /* Make sure we're fed valid data. */
        if (text.size() > MAXKEY_BUF
            || FormStartPositions_.size() + 1 > MaxFormsCount
            || FormsBuffer_.size() + text.size() + 2 > MaxFormsBufferSize)
        {
            return false;
        }

        Y_ASSERT((language != LANG_UNK) == !!(flags & FORM_HAS_LANG));
        flags &= (FORM_TRANSLIT | FORM_HAS_LANG);

        FormStartPositions_.push_back(AppendToFormsBuffer(static_cast<ui8>(language), static_cast<ui8>(flags), text));
        return true;
    }

    void PopBackForm() {
        Y_ASSERT(FormStartPositions_.size() > 0);
        FormsBuffer_.resize(FormStartPositions_.back());
        FormStartPositions_.pop_back();
    }

    void Clear() {
        LemmaBuffer_.clear();
        FormsBuffer_.clear();
        FormStartPositions_.clear();
    }

    friend bool operator==(const TDecodedKey& l, const TDecodedKey& r) {
        return l.LemmaBuffer_ == r.LemmaBuffer_ && l.FormsBuffer_ == r.FormsBuffer_;
    }

    friend bool operator!=(const TDecodedKey& l, const TDecodedKey& r) {
        return !operator==(l, r);
    }

    friend bool operator<(const TDecodedKey& l, const TDecodedKey& r) {
        int cmp = l.LemmaBuffer_.compare(r.LemmaBuffer_);
        if (cmp != 0) {
            return cmp < 0;
        }
        size_t len = Min(l.FormCount(), r.FormCount());
        for (size_t i = 0; i < len; ++i) {
            TDecodedFormRef lf = l.Form(i);
            TDecodedFormRef rf = r.Form(i);
            if (lf == rf) {
                continue;
            }
            return lf < rf;
        }
        return l.FormCount() < r.FormCount();
    }

private:
    ui16 AppendToFormsBuffer(ui8 byte0, ui8 byte1, const TStringBuf& tail) {
        ui16 startPosition = FormsBuffer_.size();
        FormsBuffer_.append(static_cast<char>(byte0));
        FormsBuffer_.append(static_cast<char>(byte1));
        FormsBuffer_.append(tail);
        Y_ENSURE(FormsBuffer_.size() <= MaxFormsBufferSize);
        return startPosition;
    }

private:

    TString LemmaBuffer_;
    TString FormsBuffer_;
    TVector<ui16> FormStartPositions_;
};


IOutputStream& operator<<(IOutputStream& stream, const TDecodedKey& key);


} // namespace NDoom
