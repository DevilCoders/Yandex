#pragma once

#include <kernel/keyinv/invkeypos/keychars.h>
#include <library/cpp/langs/langs.h>

namespace NDoom {

class TDecodedFormRef {
private:
    static constexpr TStringBuf EmptyForm = TStringBuf("\0\0", 2);

public:
    TDecodedFormRef(const TStringBuf& formBuffer)
        : Buffer_(formBuffer)
    {
        Y_ASSERT(Buffer_.size() >= 2);
    }

    TDecodedFormRef(): TDecodedFormRef(EmptyForm) {}

    operator bool () const {
        return Buffer() != EmptyForm;
    }

    TStringBuf Text() const {
        return Buffer_.SubStr(2);
    }

    EFormFlags Flags() const {
        return static_cast<EFormFlag>(Buffer_[1]);
    }

    ELanguage Language() const {
        return static_cast<ELanguage>(Buffer_[0]);
    }

    friend bool operator==(const TDecodedFormRef& l, const TDecodedFormRef& r) {
        return l.Buffer_ == r.Buffer_;
    }

    friend bool operator<(const TDecodedFormRef& l, const TDecodedFormRef& r) {
        int cmp = l.Text().compare(r.Text());
        if (cmp != 0) {
            return cmp < 0;
        }
        if (l.Flags() != r.Flags()) {
            return l.Flags() < r.Flags();
        }
        return l.Language() < r.Language();
    }

    const TStringBuf& Buffer() const {
        return Buffer_;
    }

private:
    TStringBuf Buffer_;
};

} // namespace NDoom
