#pragma once
#include "stack.h"
#include <kernel/common_server/util/accessor.h>

namespace NCS {
    class TJsonScannerByPath {
    private:
        const NJson::TJsonValue& StartNode;
        TVector<TString> CurrentPath;
        TVector<TStackContext> CurrentStack;
        TJsonScannerContext ContextInfo;
        mutable bool DataFoundFlag = false;
    protected:
        virtual bool Execute(const NJson::TJsonValue& value) const final {
            DataFoundFlag = true;
            return DoExecute(value);
        }
        virtual bool DoExecute(const NJson::TJsonValue& value) const = 0;
        const TJsonScannerContext& GetContextInfo() const {
            return ContextInfo;
        }
    public:
        bool IsDataFound() const {
            return DataFoundFlag;
        }

        TJsonScannerContext& MutableContextInfo() {
            return ContextInfo;
        }
        TJsonScannerByPath(const NJson::TJsonValue& jsonDoc)
            : StartNode(jsonDoc)
        {

        }

        bool Scan(const TString& path);
    };
}
