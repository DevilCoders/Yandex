#pragma once

#include "accessor.h"

#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/json/writer/json_value.h>

#include <util/generic/map.h>
#include <util/generic/yexception.h>
#include <util/stream/str.h>

class TCodedException : public yexception {
    CS_ACCESS(TCodedException, int, Code, HTTP_OK);
    CSA_DEFAULT(TCodedException, TString, ErrorCode);
    CSA_DEFAULT(TCodedException, TString, ErrorMessage);
    CSA_DEFAULT(TCodedException, TString, DebugMessage);
    CSA_DEFAULT(TCodedException, NJson::TJsonValue, Details);
private:
    TVector<TString> SpecialErrorCodes;

public:
    TCodedException(int code)
        : Code(code)
    {}

    TCodedException& AddErrorCode(const TString& code) {
        if (!ErrorCode) {
            ErrorCode = code;
        }
        SpecialErrorCodes.emplace_back(code);
        return *this;
    }

    bool HasReport() const {
        return SpecialErrorCodes.size() || strlen(yexception::what()) || !!DebugMessage;
    }

    NJson::TJsonValue GetDetailedReport() const;
};

class TCodedError {
public:
    static const int OK = 200;

    TCodedError(int code = OK)
        : Code(code)
    {}

    template <class T>
    inline TCodedError& operator<<(const T& t) {
        Message << t;
        return *this;
    }

    inline int GetCode() const {
        return Code;
    }

    inline const TString& GetMessage() const {
        return Message.Str();
    }

    inline bool IsOk() const {
        return Code == OK;
    }
private:
    int Code;
    TStringStream Message;
};
