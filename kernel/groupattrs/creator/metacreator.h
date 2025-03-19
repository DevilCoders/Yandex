#pragma once

#include <kernel/groupattrs/metainfo.h>

#include <util/generic/noncopyable.h>
#include <util/generic/hash.h>
#include <util/generic/ptr.h>

namespace NGroupingAttrs {

class TConfig;

/* Only for C2N maps at the moment */

    class IMetaInfoCreator {
    protected:
        typedef THashMap<TString, TSimpleSharedPtr<TMetainfo> > TData;
        TData Data;
    protected:
        virtual void ParseFile(const TString& dir, const char* attrname);
        virtual TMetainfo* Metainfo(const char* attrname);

    public:
        virtual ~IMetaInfoCreator() {}
        virtual void LoadC2N(const TConfig& config, const TString& prefix) = 0;
        virtual void SaveC2N(const TString& prefix) const = 0;
        virtual TCateg AddCateg(const char* attrname, const char* categname) = 0;
    };

    class TMetainfoCreator: public IMetaInfoCreator, TNonCopyable {
    public:
        TMetainfoCreator() {}
        ~TMetainfoCreator() override {}

        void LoadC2N(const TConfig& config, const TString& prefix) override;
        void SaveC2N(const TString& prefix) const override;

        TCateg AddCateg(const char* attrname, const char* categname) override;
    };
}
