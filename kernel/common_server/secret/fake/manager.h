#pragma once
#include "config.h"

namespace NCS {
    namespace NSecret {
        class TFakeManager: public ISecretsManager {
        protected:
            virtual bool DoEncode(TVector<TDataToken>& data, const bool writeIfNotExists) const override;
            virtual bool DoDecode(TVector<TDataToken>& data, const bool forceCheckExistance) const override;
        public:
            virtual bool SecretsStart() override {
                return true;
            }
            virtual bool SecretsStop() override {
                return true;
            }
        };
    }
}

