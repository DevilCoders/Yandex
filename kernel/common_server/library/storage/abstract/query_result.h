#pragma once
#include <util/generic/ptr.h>
#include "statement.h"
#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/library/storage/query/request.h>

namespace NCS {
    namespace NStorage {
        enum class EFetchResult {
            Success,
            Error,
            Initialized,
            NotInitialized
        };

        class IQueryResult {
        public:
            using TPtr = TAtomicSharedPtr<IQueryResult>;
        public:
            virtual ~IQueryResult() {}
            virtual ui32 GetAffectedRows() const = 0;
            virtual bool IsSucceed() const = 0;
        };

        class TQueryResult: public IQueryResult {
        public:
            TQueryResult() = default;

            TQueryResult(const bool success, const ui32 rows)
                : Succeed(success)
                , AffectedRows(rows) {
            }

            virtual ui32 GetAffectedRows() const override {
                return AffectedRows;
            }

            virtual bool IsSucceed() const override {
                return Succeed;
            }

        private:
            bool Succeed = false;
            ui32 AffectedRows = 0;
        };

        class TTransactionQueryResult {
        private:
            IQueryResult::TPtr Object;

            TTransactionQueryResult() = default;
        public:
            static TTransactionQueryResult BuildEmpty() {
                return TTransactionQueryResult();
            }

            bool Initialized() const {
                return !!Object;
            }

            TTransactionQueryResult(IQueryResult::TPtr object)
                : Object(object) {
                Y_ASSERT(!!Object);
            }

            template <class T>
            TTransactionQueryResult(TAtomicSharedPtr<T> object) {
                Object = object;
                Y_ASSERT(!!Object);
            }

            bool operator!() const {
                Y_ASSERT(!!Object);
                return !Object || !Object->IsSucceed();
            }

            ui32 GetAffectedRows() const {
                Y_ASSERT(!!Object);
                if (!Object) {
                    return 0;
                }
                return Object->GetAffectedRows();
            }
            bool IsSucceed() const {
                Y_ASSERT(!!Object);
                if (!Object) {
                    return false;
                }
                return Object->IsSucceed();
            }

            const IQueryResult* operator->() const {
                CHECK_WITH_LOG(Object);
                return Object.Get();
            }
        };

        class IFutureQueryResult {
        private:
            CSA_DEFAULT(IFutureQueryResult, TString, TransactionId);
        protected:
            virtual TTransactionQueryResult DoFetch(const ui32* requestAddress) = 0;
            virtual bool DoAddRequest(TStatement::TPtr /*statement*/, ui32* /*requestAddress*/) {
                return false;
            }

        public:
            virtual ~IFutureQueryResult() = default;
            using TPtr = TAtomicSharedPtr<IFutureQueryResult>;
            TTransactionQueryResult Fetch(const ui32* requestAddress = nullptr) noexcept;
            bool AddRequest(TStatement::TPtr statement, ui32* requestAddress = nullptr) noexcept;
            bool AddRequest(const TStatement& statement, ui32* requestAddress = nullptr) noexcept;
            bool AddQuery(const NRequest::IExternalMethods& extMethods, const TSRQuery& query, ui32* requestAddress = nullptr) noexcept;
        };

        class TSimpleFutureQueryResult: public IFutureQueryResult {
        private:
            IQueryResult::TPtr FetchResult;
        protected:
            virtual TTransactionQueryResult DoFetch(const ui32* /*requestAddress*/) override {
                return FetchResult;
            }
        public:
            TSimpleFutureQueryResult(IQueryResult::TPtr result)
                : FetchResult(result) {

            }
        };

    }
}
