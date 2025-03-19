#pragma once

#include "config.h"
#include "docsattrsdata.h"

#include <library/cpp/on_disk/chunks/chunked_helpers.h>

#include <util/system/defaults.h>

namespace NMemoryAttrMap {
class TOneDocAttrs;
}

namespace NGroupingAttrs {

class TMutableDocAttrs;

class IDocsAttrsWriter {
public:
    IDocsAttrsWriter(const TConfig& config);
    virtual ~IDocsAttrsWriter() {};

    void Write(TMutableDocAttrs& docAttrs);
    void Write(const NMemoryAttrMap::TOneDocAttrs& docAttrs, const TVector<ui32>& remap);
    void WriteEmpty();

    void Close();

    static const TVersion DefaultVersion;
    static const TVersion DefaultWritableVersion;
    static const TVersion WadVersion;

protected:
    const TConfig& Config;
    //must init it in constructor
    THolder<IDocsAttrsDataWriter> DataWriter;

    virtual void Finalize() {};
};

class TDocsAttrsWriter : public IDocsAttrsWriter {
public:
    TDocsAttrsWriter(const TConfig& config, TVersion version, TConfig::Mode format, const TString& tmpdir, IOutputStream& out, const char* prefix);
    TDocsAttrsWriter(const TConfig& config, TVersion version, TConfig::Mode format, const TString& tmpdir, const TString& output, const char* prefix);

protected:
    void Finalize() override;

public:
    using IDocsAttrsWriter::DefaultVersion;
    using IDocsAttrsWriter::DefaultWritableVersion;

private:
    THolder<IOutputStream> OutputStream;
    TNamedChunkedDataWriter Writer;
};

class TDocsAttrsWadWriter : public IDocsAttrsWriter {
public:
    TDocsAttrsWadWriter(const TConfig& config, const TString& output);
};

class TDocsAttrs64WadWriter : public IDocsAttrsWriter {
public:
    TDocsAttrs64WadWriter(const TConfig& config, const TString& output);
};

}
