#include "responseimpl.h"

#include "initexc.h"

namespace NBlackbox2 {
    TResponse::TImpl::TImpl(const TStringBuf response) {
        Xmlroot_.Reset(new xmlConfig::XConfig);
        if (Xmlroot_->Parse(response)) {
            throw TFatalError(NSessionCodes::UNKNOWN,
                              "Failed to parse blackbox response: " + TString(response));
        }
        try {
            Doc_.Reset(new xmlConfig::Part(Xmlroot_->GetFirst("/doc")));
        } catch (const TInitExc& e) {
            throw TFatalError(NSessionCodes::UNKNOWN,
                              "Missing root <doc> node: " + TString(response));
        }
    }

    TResponse::TImpl::TImpl(const xmlConfig::Part* part) {
        if (!part)
            throw TFatalError(NSessionCodes::UNKNOWN,
                              "Unable to create response from NULL XML part");

        Doc_.Reset(new xmlConfig::Part(*part));
    }

    TResponse::TImpl::~TImpl() {
    }

    bool TResponse::TImpl::GetIfExists(const TString& xpath, TString& variable) {
        return Doc_->GetIfExists(xpath.c_str(), variable);
    }

    bool TResponse::TImpl::GetIfExists(const TString& xpath, long int& variable) {
        return Doc_->GetIfExists(xpath.c_str(), variable);
    }

    xmlConfig::Parts TResponse::TImpl::GetParts(const TString& xpath) {
        return Doc_->GetParts(xpath.c_str());
    }
}
