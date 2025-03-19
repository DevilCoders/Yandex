#pragma once
#include <kernel/common_server/processors/common/handler.h>
#include <kernel/common_server/proposition/object.h>
#include <kernel/common_server/library/scheme/handler.h>
#include <kernel/common_server/processors/db_entity/handler.h>
#include "permission.h"

namespace NCS {

    namespace NHandlers {
        class TPropositionUpsertHandler: public TCommonUpsert<TPropositionUpsertHandler, NPropositions::TDBProposition, TSystemUserPermissions, IBaseServer> {
        private:
            using TBase = TCommonUpsert<TPropositionUpsertHandler, NPropositions::TDBProposition, TSystemUserPermissions, IBaseServer>;
        protected:
            using TBase::ConfigHttpStatus;
            using TBase::GetJsonData;
            using TBase::ReqCheckCondition;
        public:
            using TBase::TBase;

            static TString GetTypeName() {
                return "propositions-upsert";
            }

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) override;
        };

        class TPropositionRemoveHandler: public TCommonRemove<TPropositionRemoveHandler, NPropositions::TDBProposition, TSystemUserPermissions, IBaseServer> {
        private:
            using TBase = TCommonRemove<TPropositionRemoveHandler, NPropositions::TDBProposition, TSystemUserPermissions, IBaseServer>;
        public:
            using TBase::TBase;
            static TString GetTypeName() {
                return "propositions-remove";
            }

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) override;
        };

        class TPropositionInfoHandler: public TCommonImpl<TPropositionInfoHandler, TSystemUserPermissions, IBaseServer> {
        private:
            using TBase = TCommonImpl<TPropositionInfoHandler, TSystemUserPermissions, IBaseServer>;
        protected:
            virtual bool DoFillHandlerScheme(NScheme::THandlerScheme& scheme, const IBaseServer& server) const override;
        public:
            using TBase::TBase;

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;

            static TString GetTypeName() {
                return "propositions-info";
            }
        };

        class TPropositionVerdictHandler: public TCommonImpl<TPropositionVerdictHandler, TSystemUserPermissions, IBaseServer> {
        private:
            using TBase = TCommonImpl<TPropositionVerdictHandler, TSystemUserPermissions, IBaseServer>;
        protected:
            virtual bool DoFillHandlerScheme(NScheme::THandlerScheme& scheme, const IBaseServer& /*server*/) const override;
        public:
            using TBase::TBase;

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;

            static TString GetTypeName() {
                return "propositions-verdict";
            }
        };

        class TPropositionVerdictInfoHandler : public TCommonImpl<TPropositionVerdictInfoHandler, TSystemUserPermissions, IBaseServer> {
        private:
            using TBase = TCommonImpl<TPropositionVerdictInfoHandler, TSystemUserPermissions, IBaseServer>;
        protected:
            virtual bool DoFillHandlerScheme(NScheme::THandlerScheme& scheme, const IBaseServer& server) const override;
        public:
            using TBase::TBase;

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;

            static TString GetTypeName() {
                return "propositions-verdict-info";
            }

            static NFrontend::TScheme GetExtendedPropositionScheme(const IBaseServer& server);
        };

        class TPropositionHistoryInfoHandler: public NCS::NHandlers::TDBObjectsHistoryInfoImpl<TPropositionHistoryInfoHandler, NPropositions::TDBProposition, TSystemUserPermissions, IBaseServer> {
        private:
            using TBase = NCS::NHandlers::TDBObjectsHistoryInfoImpl<TPropositionHistoryInfoHandler, NPropositions::TDBProposition, TSystemUserPermissions, IBaseServer>;
        protected:
            virtual const IDirectObjectsOperator<NPropositions::TDBProposition>* GetObjectsManager() const override;

            virtual bool CheckObjectVisibility(TAtomicSharedPtr<IUserPermissions> permissions, const NPropositions::TDBProposition& object) const override {
                return permissions->Check<NCS::NHandlers::TPropositionPermissions>(NCS::NHandlers::EPropositionAction::HistoryObserve, object.GetProposedObject());
            }
        public:
            using TBase::TBase;
            static TString GetTypeName() {
                return "propositions-history-info";
            }
        };

        class TPropositionVerdictHistoryInfoHandler: public NCS::NHandlers::TDBObjectsHistoryInfoImpl<TPropositionVerdictHistoryInfoHandler, NPropositions::TDBVerdict, TSystemUserPermissions, IBaseServer> {
        private:
            using TBase = NCS::NHandlers::TDBObjectsHistoryInfoImpl<TPropositionVerdictHistoryInfoHandler, NPropositions::TDBVerdict, TSystemUserPermissions, IBaseServer>;
        protected:
            virtual const IDirectObjectsOperator<NPropositions::TDBVerdict>* GetObjectsManager() const override;

            virtual bool CheckObjectVisibility(TAtomicSharedPtr<IUserPermissions> permissions, const NPropositions::TDBVerdict& object) const override {
                return permissions->Check<NCS::NHandlers::TPropositionPermissions>(NCS::NHandlers::EPropositionAction::HistoryObserve, object);
            }
        public:
            using TBase::TBase;
            static TString GetTypeName() {
                return "propositions-verdict-history-info";
            }
        };
    }
}
