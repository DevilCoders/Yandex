#include "rsa.h"

#include <kernel/common_server/library/openssl/aes.h>

#include <util/string/join.h>

namespace {
    TString ToPemStr(const TString& str) {
        TVector<TString> list = StringSplitter(str).SplitByString("-----").ToList<TString>();
        if (list.size() != 5)
            return str;

        list[2] = JoinSeq("\n", StringSplitter(list[2]).SplitByString(" ").ToList<TString>());
        return JoinSeq("-----", list);
    }
}

namespace NCS {
    TRSAKeyCipherConfig::TFactory::TRegistrator<TRSAKeyCipherConfig> TRSAKeyCipherConfig::Registrator("rsa");

    TRSACipher::TRSACipher(const TCipherKey& key, const TMaybeFail<TCipherKey>& pendingKey, const TString& privKeyStr)
        : TBase(key, pendingKey)
    {
        PubKey.Init(ToPemStr(key.GetValue()));
        SetPrivateKey(privKeyStr);
    }

    bool TRSACipher::Encrypt(const TString& plain, TString& encrypted) const {
        if (!!PrivKey) {
            return PrivKey->Encrypt(plain, encrypted);
        }
        return PubKey.Encrypt(plain, encrypted);
    }

    bool TRSACipher::Decrypt(const TString& encrypted, TString& plain) const {
        if (!PrivKey) {
            return false;
        }
        return PrivKey->Decrypt(encrypted, plain);
    }

    void TRSACipher::SetPrivateKey(const TString& privKeyStr) {
        if (!!privKeyStr) {
            PrivKey = NOpenssl::TRSAPrivateKey();
            PrivKey->Init(ToPemStr(privKeyStr));
        }
    }

    void TRSAKeyCipherConfig::DoInit(const TYandexConfig::Section* section) {
        TBase::DoInit(section);
        PrivKeyPath = section->GetDirectives().Value<TString>("PrivKeyPath", PrivKeyPath);
        if (PrivKeyPath) {
            PrivKey = TFileInput(PrivKeyPath).ReadAll();
        }

        if (!PrivKey) {
            PrivKey = section->GetDirectives().Value<TString>("PrivKey", PrivKey);
        }
    }

    void TRSAKeyCipherConfig::DoToString(IOutputStream& os) const {
        TBase::DoToString(os);
        os << "PrivKeyPath: " << PrivKeyPath << Endl;
    }

    IAbstractCipher::TPtr TRSAKeyCipherConfig::DoConstruct(const IBaseServer* /*server*/) const {
        return MakeAtomicShared<TRSACipher>(GetKey(), GetPendingKey(), PrivKey);
    }
}
