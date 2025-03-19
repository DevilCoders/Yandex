#pragma once

#include <kernel/common_server/settings/abstract/abstract.h>

namespace NCS {

    class ITemplate {
    public:
        virtual TString Apply(const TString& source) const = 0;
        virtual ~ITemplate() = default;
    };

    class IVariableTemplate: public ITemplate {
    protected:
        virtual bool GetVariable(const TString& field, TString& result) const = 0;

    public:
        virtual TString Apply(const TString& source) const override;
    };

    class TTemplateSettings: public IVariableTemplate {
    private:
        CSA_DEFAULT(TTemplateSettings, TString, Prefix);
        const ISettings& Settings;
    
    protected:
        virtual bool GetVariable(const TString& field, TString& result) const override;

    public:
        TTemplateSettings(const TString& prefix, const ISettings& settings)
            : Prefix(prefix)
            , Settings(settings)
        {
        }
    };
}