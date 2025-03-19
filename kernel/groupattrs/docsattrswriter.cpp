#include "docsattrswriter.h"

#include "attrmap.h"
#include "mutdocattrs.h"

namespace NGroupingAttrs {

const TVersion IDocsAttrsWriter::WadVersion = 5;
const TVersion IDocsAttrsWriter::DefaultVersion = 4;
const TVersion IDocsAttrsWriter::DefaultWritableVersion = 3;

IDocsAttrsWriter::IDocsAttrsWriter(const TConfig& config)
    : Config(config)
{
}

TDocsAttrsWriter::TDocsAttrsWriter(const TConfig& config, TVersion version, TConfig::Mode format, const TString& tmpdir, IOutputStream& out, const char* prefix)
    : IDocsAttrsWriter(config)
    , Writer(out)
{
    DataWriter = std::move(CreateDocsAttrsDataWriter(config, version, static_cast<TFormat>(format),
                                               tmpdir, prefix, Writer));
}

TDocsAttrsWriter::TDocsAttrsWriter(const TConfig& config, TVersion version, TConfig::Mode format, const TString& tmpdir, const TString& output, const char* prefix)
    : IDocsAttrsWriter(config)
    , OutputStream(new TFixedBufferFileOutput(output))
    , Writer(*OutputStream)
{
    DataWriter = std::move(CreateDocsAttrsDataWriter(config, version, static_cast<TFormat>(format),
                                               tmpdir, prefix, Writer));
}

void IDocsAttrsWriter::Write(TMutableDocAttrs& docAttrs) {
    Y_ASSERT(Config == docAttrs.Config());

    for (ui32 attrNum = 0; attrNum < Config.AttrCount(); ++attrNum) {
        const char* name = Config.AttrName(attrNum);
        bool unique = Config.IsAttrUnique(attrNum);
        TCategSeries* categs = docAttrs.Attrs(name);
        categs->Sort();

        DataWriter->WriteAttr(attrNum, unique, *categs);
    }

    DataWriter->NextDoc();
}

void IDocsAttrsWriter::Write(const NMemoryAttrMap::TOneDocAttrs& docAttrs, const TVector<ui32>& remap) {
    for (ui32 attrNum = 0; attrNum < remap.size(); ++attrNum) {
        ui32 originalAttrNum = remap[attrNum];
        bool unique = Config.IsAttrUnique(attrNum);
        TConfig::Type type = Config.AttrType(attrNum);
        TCategSeries categs;
        const char* begin;
        const char* end;
        docAttrs.SetBeginEnd(originalAttrNum, begin, end);
        docAttrs.GetCategs(type, begin, end, &categs);
        categs.Sort();

        DataWriter->WriteAttr(attrNum, unique, categs);
    }

    DataWriter->NextDoc();
}

void IDocsAttrsWriter::WriteEmpty() {
    DataWriter->WriteEmpty();
    DataWriter->NextDoc();
}

void IDocsAttrsWriter::Close() {
    DataWriter->CloseData();
    DataWriter->WriteConfig();
    DataWriter->WriteFormat();
    DataWriter->WriteVersion();
    Finalize();
}

void TDocsAttrsWriter::Finalize() {
    Writer.WriteFooter();
}

TDocsAttrsWadWriter::TDocsAttrsWadWriter(const TConfig& config, const TString& output)
    : IDocsAttrsWriter(config) {
    DataWriter = std::move(CreateWadDocsAttrsDataWriter(config, output));
}

TDocsAttrs64WadWriter::TDocsAttrs64WadWriter(const TConfig& config, const TString& output)
    : IDocsAttrsWriter(config) {
    DataWriter = std::move(CreateWadDocsAttrs64DataWriter(config, output));
}

}
