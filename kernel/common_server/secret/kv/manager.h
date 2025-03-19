#pragma once
#include "config.h"
#include <kernel/common_server/api/history/db_entities.h>

namespace NCS {
    namespace NSecret {
        class TKVManager: public ISecretsManager {
        private:
            const NKVStorage::IStorage::TPtr Storage;
            const bool AllowDuplicate;
        protected:
            virtual bool DoEncode(TVector<TDataToken>& data, const bool writeIfNotExists) const override;
            virtual bool DoDecode(TVector<TDataToken>& data, const bool forceCheckExistance) const override;
        public:
            TKVManager(const NKVStorage::IStorage::TPtr storage, const bool allowDuplicate) 
                : Storage(storage)
                , AllowDuplicate(allowDuplicate)
            {}
            virtual bool SecretsStart() override {
                return true;
            }
            virtual bool SecretsStop() override {
                return true;
            }
        };
    }
}
