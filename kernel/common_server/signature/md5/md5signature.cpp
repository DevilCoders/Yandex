#include "md5signature.h"
#include <library/cpp/digest/md5/md5.h>

namespace NCS {

    TElSignatureMD5Config::TFactory::TRegistrator<TElSignatureMD5Config> TElSignatureMD5Config::Registrator(TElSignatureMD5Config::GetTypeName());

    bool TElSignatureMD5::MakeSignature(const TString& originalData, TString& signature) const {
        signature = MD5::Calc(originalData);
        return true;
    }

    TAtomicSharedPtr<NCS::IElSignature> TElSignatureMD5Config::Construct(const IBaseServer& /*server*/) const {
        return MakeAtomicShared<TElSignatureMD5>();
    }

    TElSignatureXMLMD5Config::TFactory::TRegistrator<TElSignatureXMLMD5Config> TElSignatureXMLMD5Config::Registrator(TElSignatureXMLMD5Config::GetTypeName());

    bool TElSignatureXMLMD5::MakeSignature(const TString& originalData, TString& signature) const {
        signature = MD5::Calc(originalData);
        return true;
    }

    bool TElSignatureXMLMD5::FillSignatureInfo(const TString& /*strForSignature*/, NXml::TNode& /*signatureInfo*/) const {
        return true;
    }

    TAtomicSharedPtr<NCS::IElSignature> TElSignatureXMLMD5Config::Construct(const IBaseServer& /*server*/) const {
        return MakeAtomicShared<TElSignatureXMLMD5>();
    }
}
