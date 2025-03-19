#pragma once

#include <kernel/common_server/ciphers/abstract.h>
#include <kernel/common_server/ciphers/config.h>
#include <kernel/common_server/library/tvm_services/abstract/abstract.h>

namespace NCS {
    class TChainCipher: public IAbstractCipher {
    private:
        using TBase = IAbstractCipher;

        CSA_READONLY_DEF(TVector<IAbstractCipher::TPtr>, Chain);

    protected:
        virtual IAbstractCipher::TPtr DoCreateNewVersion() const override {
            TVector<IAbstractCipher::TPtr> newChain;
            for (const auto& i : Chain) {
                auto newCipher = i->CreateNewVersion();
                newChain.emplace_back(!!newCipher ? newCipher : i);
            }
            return MakeAtomicShared<TChainCipher>(std::move(newChain));
        }

    public:
        virtual bool Encrypt(const TString& plain, TString& encrypted) const final;
        virtual bool Decrypt(const TString& encrypted, TString& plain) const final;

        TChainCipher(TVector<IAbstractCipher::TPtr>&& chain)
            : Chain(std::move(chain))
        {
            CHECK_WITH_LOG(!Chain.empty());
        }

        virtual bool NeedRotate() const override {
            bool res = false;
            for (const auto& i : Chain) {
                res |= i->NeedRotate();
            }
            return res;
        }
    };

    class TChainCipherConfig: public ICipherConfig {
    private:
        using TBase = ICipherConfig;
        CSA_READONLY_DEF(TVector<TString>, ChainNames);

        static TFactory::TRegistrator<TChainCipherConfig> Registrator;

    protected:
        void DoInit(const TYandexConfig::Section* section) final;
        void DoToString(IOutputStream& os) const final;

    public:
        virtual IAbstractCipher::TPtr DoConstruct(const IBaseServer* server) const final;

        using TBase::TBase;
    };
}
