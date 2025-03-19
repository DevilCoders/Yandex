#pragma once

#include "metacreator.h"

#include <kernel/groupattrs/config.h>
#include <kernel/groupattrs/docsattrswriter.h>

#include <kernel/search_types/search_types.h>

#include <util/generic/noncopyable.h>
#include <util/stream/file.h>
#include <util/system/defaults.h>
#include <util/system/fs.h>

namespace NGroupingAttrs {

class TMutableDocAttrs;

class ICreator {
public:
    virtual TCateg ConvertAttrValue(const char* attrname, const char* value) = 0;

    virtual TConfig& Config() = 0;
    virtual void AddDoc(TMutableDocAttrs& docAttrs) = 0;
    virtual void MakePortion() {}
    virtual void SaveC2N(const TString& /*prefix*/) const {}
    virtual void LoadC2N(const TString& /*prefix*/) {}
    virtual void Init(const TString& /*grconf*/, const TString& /*filename*/) {}
    virtual void Close() {}
    virtual ~ICreator() {}
};

class TCreator : public ICreator, TNonCopyable {
private:
    TConfig Config_;
    IMetaInfoCreator* MetaCreator;
    THolder<TMetainfoCreator> DefaultMetaCreatorStorage;

    TString Path;
    TString Filename;
    ui32 PortionsCount;

    THolder<TFixedBufferFileOutput> Out;
    THolder<TDocsAttrsWriter> Writer;

    TConfig::Mode ResultWriteMode;
    TVersion ResultVersion;
    TVersion PortionVersion;

    bool IsOpen;

private:
    bool HasOpenPortion() const;
    void OpenPortion();
    void RemovePortions();

public:
    TCreator(TConfig::Mode resultWriteMode = TConfig::Index,
             IMetaInfoCreator* metaInfoCreator = nullptr,
             TVersion resultVersion = TDocsAttrsWriter::DefaultVersion,
             TVersion portionVersion = TDocsAttrsWriter::DefaultVersion)
        : Config_(TConfig::Index)
        , PortionsCount(0)
        , ResultWriteMode(resultWriteMode)
        , ResultVersion(resultVersion)
        , PortionVersion(portionVersion)
        , IsOpen(false)
    {
        if (metaInfoCreator) {
            MetaCreator = metaInfoCreator;
        } else {
            DefaultMetaCreatorStorage.Reset(new TMetainfoCreator);
            MetaCreator = DefaultMetaCreatorStorage.Get();
        }
    }

    ~TCreator() override {
        Close();
    }

    TConfig& Config() override {
        return Config_;
    }

    void Init(const TString& grconf, const TString& filename) override;
    void InitFromFile(const TString& grconffilename, const TString& filename);

    TCateg ConvertAttrValue(const char* attrname, const char* value) override;

    void AddDoc(TMutableDocAttrs& docAttrs) override;
    void MakePortion() override;

    void SaveC2N(const TString& prefix) const override;
    void LoadC2N(const TString& prefix) override;
    void Close() override;
};

}
