#include "proto_dict.h"

namespace {
    void CheckUtf16String(const TStringBuf& str) {
        size_t len = str.length();
        Y_ENSURE(len % 2 == 0);
        if (len > 0) {
            Y_ENSURE(str[len - 1] == 0);
            Y_ENSURE(str[len - 2] == 0);
        }
    }
}

namespace NNewLemmer {
    const wchar16* TProtoScheme::GetLemmaFlex() const {
        if (Y_LIKELY(Scheme->HasFlex())) {
            CheckUtf16String(TStringBuf(Scheme->GetFlex().GetText().data(), Scheme->GetFlex().GetText().length()));
            return reinterpret_cast<const wchar16*>(Scheme->GetFlex().GetText().data());
        } else {
            return GetEmptyFlex();
        }
    }

    const wchar16* TProtoBlock::GetFormFlex() const {
        if (Block->HasFlex()) {
            CheckUtf16String(TStringBuf(Block->GetFlex().GetText().data(), Block->GetFlex().GetText().length()));
            return reinterpret_cast<const wchar16*>(Block->GetFlex().GetText().data());
        } else {
            return GetEmptyFlex();
        }
    }
}
