#pragma once

#include <kernel/doom/key/key_decoder.h>

#include <ysite/yandex/posfilter/key_form.h>

#include <util/string/util.h>

namespace NReqBundleIteratorImpl {
    class TOffroadPackedKey {
    public:
        bool Init(const TStringBuf& packedKey) {
            return NDoom::TKeyDecoder().Decode(packedKey, &Key_);
        }

        int FormsCount() const {
            return Key_.FormCount();
        }

        // returns false if the form is empty, aka "hole"
        bool Form(size_t index, TKeyForm* result) {
            Y_ASSERT(result);
            NDoom::TDecodedFormRef form = Key_.Form(index);
            // Still need to init result in case it is used
            result->Init(form.Text(), form.Language(), form.Flags());
            return form;
        }

        ui8 AdditionalTokensNumberForForm(size_t /*index*/) const {
            return 0;
        }

    private:
        NDoom::TDecodedKey Key_;
    };
} // namespace NReqBundleIteratorImpl
