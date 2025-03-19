#pragma once

#include "config.h"
#include "object.h"
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/api/history/db_entities.h>
#include <kernel/common_server/obfuscator/obfuscators/abstract.h>
#include <kernel/common_server/obfuscator/obfuscators/total.h>

namespace NCS {
    namespace NObfuscator {

        class TObfuscatorsPool {
        private:
            using EContentType = IObfuscator::EContentType;
            using TObfuscatorByContentType = TMap<EContentType, TVector<TDBObfuscator>>;
            CSA_READONLY_DEF(TObfuscatorByContentType, Objects);

            void AddDefaultObfuscators(TVector<TDBObfuscator>& objects) const;

        public:
            TObfuscatorsPool(const TVector<TDBObfuscator>& objects);
        };

        class TDBManager: public TDBMetaEntitiesManager<TDBObfuscator, TDBEntitiesSnapshotsConstructor<TDBObfuscator, TObfuscatorsPool>>, public virtual IObfuscatorManager {
        private:
            using TBase = TDBMetaEntitiesManager<TDBObfuscator, TDBEntitiesSnapshotsConstructor<TDBObfuscator, TObfuscatorsPool>>;
            using EContentType = IObfuscator::EContentType;

            const TDBManagerConfig Config;
            IObfuscator::TPtr DefaultObfuscator;

            bool DisabledBySettings() const {
                return ICSOperator::GetServer().GetSettings().GetValueDef("obfuscator.disabled", false);
            }

            IObfuscator::TPtr GetDefaultObfuscator(const TObfuscatorKey& key) const;

        protected:
            virtual bool ParsingStrictValidation() const override {
                return ICSOperator::GetServer().GetSettings().GetValueDef("validations." + TDBObfuscator::GetTableName() + ".parsing", false);
            }

        public:
            using TPtr = TAtomicSharedPtr<TDBManager>;

            virtual bool Start() noexcept override {
                return TBase::Start();
            }

            virtual bool Stop() noexcept override {
                return TBase::Stop();
            }

            virtual IObfuscator::TPtr GetObfuscatorFor(const TObfuscatorKey& key) const override;

            TDBManager(NStorage::IDatabase::TPtr db, const TDBManagerConfig& config)
                : TBase(db, config.GetHistoryConfig())
                , Config(config)
                , DefaultObfuscator(Config.GetTotalObfuscateByDefault() ? MakeAtomicShared<TTotalObfuscator>() : nullptr)
            {
            }

            TVector<TDBObfuscator> GetObjectsByContentType(const EContentType key) const;
        };

        class TFakeObfuscatorManager: public IObfuscatorManager {
        public:
            virtual IObfuscator::TPtr GetObfuscatorFor(const TObfuscatorKey& /*key*/) const override {
                return nullptr;
            }
            virtual bool Start() noexcept override {
                return true;
            }
            virtual bool Stop() noexcept override {
                return true;
            }
        };
    }
}
