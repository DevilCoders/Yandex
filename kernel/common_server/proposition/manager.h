#pragma once
#include "config.h"
#include "object.h"
#include "verdict.h"

#include <library/cpp/http/misc/httpreqdata.h>

#include <kernel/common_server/api/history/db_entities.h>
#include <kernel/common_server/library/searchserver/simple/context/replier.h>

namespace NCS {
    namespace NPropositions {
        class TDBVerdictsManager: public TDBEntitiesCache<TDBVerdict> {
        private:
            using TBase = TDBEntitiesCache<TDBVerdict>;
        public:

            using TBase::TBase;

            bool RemoveByPropositionId(const ui32 propositionId, const TString& userId, NCS::TEntitySession& session) const {
                return RemoveObjectsBySRCondition(
                    TSRCondition().Init<TSRBinary>("proposition_id", propositionId)
                    , userId, session);
            }

            bool GetVerdicts(const TDBProposition& proposition, TVector<TDBVerdict>& verdicts, NCS::TEntitySession& session) const;
            bool Verdict(const TDBProposition& proposition, const TMaybe<TDBVerdict>& verdict, const TString& userId, NCS::TEntitySession& session, EVerdict& finalVerdict) const;
        };

        class TDBManager: public TDBEntitiesManager<TDBProposition> {
            CSA_READONLY_DEF(TDBManagerConfig, Config);
        private:
            using TBase = TDBEntitiesManager<TDBProposition>;
            class TIndexByPropositionObjectIdPolicy {
            public:
                using TKey = TString;
                using TObject = TDBProposition;
                static const TString& GetKey(const TObject& object) {
                    return object.GetPropositionObjectId();
                }
                static ui32 GetUniqueId(const TObject& object) {
                    return object.GetPropositionId();
                }
            };

            class TIndexByPropositionCategoryIdPolicy {
            public:
                using TKey = TString;
                using TObject = TDBProposition;
                static const TString& GetKey(const TObject& object) {
                    return object.GetPropositionCategoryId();
                }
                static ui32 GetUniqueId(const TObject& object) {
                    return object.GetPropositionId();
                }
            };

            using TIndexByPropositionObjectId = TObjectByKeyIndex<TIndexByPropositionObjectIdPolicy >;
            mutable TIndexByPropositionObjectId IndexByPropositionObjectId;

            using TIndexByPropositionCategoryId = TObjectByKeyIndex<TIndexByPropositionCategoryIdPolicy>;
            mutable TIndexByPropositionCategoryId IndexByPropositionCategoryId;

            CS_HOLDER(TDBVerdictsManager, Verdicts);
            const IBaseServer& Server;

            bool ApplyFinalVerdict(const TDBProposition& proposition, const EVerdict verdict, const TString& userId, NCS::TEntitySession& session, IReplyContext::TPtr requestContext) const;
        protected:
            using TBase::Objects;


            virtual bool OnBeforeRemoveObject(const ui32& propositionId, const TString& userId, NCS::TEntitySession& session) const override {
                return Verdicts->RemoveByPropositionId(propositionId, userId, session);
            }

            virtual bool OnAfterUpsertObject(const TDBProposition& proposition, const TString& userId, NCS::TEntitySession& session) const override {
                EVerdict finalVerdict;
                if (!Verdicts->Verdict(proposition, Nothing(), userId, session, finalVerdict)) {
                    return false;
                }
                return ApplyFinalVerdict(proposition, finalVerdict, userId, session, nullptr);
            }

            virtual bool DoRebuildCacheUnsafe() const override;
            virtual void DoAcceptHistoryEventBeforeRemoveUnsafe(const TObjectEvent<TDBProposition>& ev) const override;
            virtual void DoAcceptHistoryEventAfterChangeUnsafe(const TObjectEvent<TDBProposition>& ev, TDBProposition& object) const override;
            virtual bool DoStart() override {
                if (!TBase::DoStart()) {
                    return false;
                }
                if (!Verdicts->Start()) {
                    return false;
                }
                return true;
            }

            virtual bool DoStop() override {
                if (!Verdicts->Stop()) {
                    return false;
                }
                if (!TBase::DoStop()) {
                    return false;
                }
                return true;
            }
        public:
            TDBManager(NStorage::IDatabase::TPtr db, const TDBManagerConfig& config, const IBaseServer& server)
                : TBase(db, config.GetHistoryConfig())
                , Config(config)
                , Server(server)
            {
                Verdicts = MakeHolder<TDBVerdictsManager>(db, config.GetHistoryConfig());
            }
            bool Verdict(const TDBVerdict& verdict, const TString& userId, NCS::TEntitySession& session, IReplyContext::TPtr requestContext) const;
            TVector<TDBProposition> GetObjectsByPropositionObjectId(const TString& objectId) const;
            TVector<TDBProposition> GetObjectsByPropositionCategoryId(const TString& objectType) const;
            const TDBVerdictsManager* GetVerdictManager() const {
                return Verdicts.Get();
            }

            bool ExecuteProposition(const TDBProposition& proposition, const TString& userId, NCS::TEntitySession& session) const;
        };
    }
}

