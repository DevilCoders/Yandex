#include "database.h"
#include <library/cpp/logger/global/global.h>
#include <util/generic/hash_set.h>
#include <util/stream/file.h>

namespace NCS {
    namespace NStorage {

        NCS::NStorage::TTransactionFeatures IDatabase::TransactionMaker() const {
            return TTransactionFeatures(*this);
        }

        ITransaction::TPtr IDatabase::CreateTransaction(const bool readonly /*= false*/) const {
            return TransactionMaker().ReadOnly(readonly).Build();
        }

        ITransaction::TPtr IDatabase::CreateTransaction(const TTransactionFeatures& features) const {
            auto result = DoCreateTransaction(features);
            if (result) {
                result->SetExpectFail(features.IsExpectFail());
            }
            return result;
        }

        ITransaction::TPtr TTransactionFeatures::Build() const {
            return Database.CreateTransaction(*this);
        }

    }
}
