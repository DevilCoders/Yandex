#include "total.h"

namespace {
    constexpr TStringBuf ObfuscatedMessage = "Hidden personal info";
}

namespace NCS {
    namespace NObfuscator {
        TTotalObfuscator::TFactory::TRegistrator<TTotalObfuscator> TTotalObfuscator::Registrator(TTotalObfuscator::GetTypeName());

        TString TTotalObfuscator::DoObfuscate(const TStringBuf /*str*/) const {
            return TString(ObfuscatedMessage);
        }
    }
}
