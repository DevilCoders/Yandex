#include "chain.h"

#include <kernel/common_server/abstract/frontend.h>

namespace NCS {
    TChainCipherConfig::TFactory::TRegistrator<TChainCipherConfig> TChainCipherConfig::Registrator("chain");

    bool TChainCipher::Encrypt(const TString& plain, TString& encrypted) const {
        TString prev(plain);
        TString next;
        for (auto i : Chain) {
            if (!i) {
                return false;
            }
            if (!i->Encrypt(prev, next)) {
                return false;
            }
            std::swap(prev, next);
        }
        encrypted = prev;
        return true;
    }

    bool TChainCipher::Decrypt(const TString& encrypted, TString& plain) const {
        TString prev(encrypted);
        TString next;
        for (auto it = Chain.rbegin(); it != Chain.rend(); ++it) {
            if (!(*it)) {
                return false;
            }
            if (!(*it)->Decrypt(prev, next)) {
                return false;
            }
            std::swap(prev, next);
        }
        plain = prev;
        return true;
    }

    void TChainCipherConfig::DoInit(const TYandexConfig::Section* section) {
        StringSplitter(section->GetDirectives().Value("ChainNames", JoinSeq(",", ChainNames))).SplitBySet(",").SkipEmpty().ParseInto(&ChainNames);
    }

    void TChainCipherConfig::DoToString(IOutputStream& os) const {
        os << "ChainNames: " << JoinSeq(",", ChainNames) << Endl;
    }

    IAbstractCipher::TPtr TChainCipherConfig::DoConstruct(const IBaseServer* server) const {
        CHECK_WITH_LOG(server);

        TVector<IAbstractCipher::TPtr> chain;
        for (const auto& i : ChainNames) {
            auto cipher = server->GetCipherPtr(i);
            CHECK_WITH_LOG(cipher);
            chain.emplace_back(std::move(cipher));
        }
        return MakeAtomicShared<TChainCipher>(std::move(chain));
    }
}
