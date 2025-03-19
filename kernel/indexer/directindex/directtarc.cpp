#include <util/system/defaults.h>
#include <util/generic/buffer.h>
#include <util/stream/output.h>
#include <library/cpp/charset/wide.h>

#include <kernel/indexer/direct_text/dt.h>

#include <ysite/directtext/textarchive/archiver.h>
#include <kernel/indexer/dtcreator/dtcreator.h>

#include "directtarc.h"
#include "directtokenizer.h"

namespace NIndexerCore {

class TExtBlockCreator : public IDirectTextCallback2 {
public:
    TExtBlockCreator(IOutputStream& out)
        : Output(out)
    {
    }

    void ProcessDirectText2(IDocumentDataInserter*, const TDirectTextData2& directText, ui32) override {
        TTextArchiveWriter writer;
        writer.AddTextEntries(directText.Entries, directText.EntryCount, directText.Zones, directText.ZoneCount, nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0);
        writer.FlushExtBlock(Output);
    }
private:
    IOutputStream& Output;
};

class TArcTextBlockCreator::TImpl {
private:
    TDTCreatorConfig Cfg;
    TDirectTextCreator DCreator;
    TDirectTokenizer DTokenizer;
    TBufferOutput Block;
    TExtBlockCreator BlockCreator;
public:
    TImpl(TLangMask langs, ELanguage langprior)
        : DCreator(Cfg, langs, langprior)
        , DTokenizer(DCreator)
        , Block(8192)
        , BlockCreator(Block)
    {
        DTokenizer.AddDoc(0, langprior);
    }

    void AddText(const wchar16* text, size_t len, const char* zoneName) {
        if (zoneName && *zoneName)
            DTokenizer.OpenZone(zoneName);
        DTokenizer.StoreText(text, len, MID_RELEV);
        if (zoneName && *zoneName)
            DTokenizer.CloseZone(zoneName);
    }

    void CommitDocument(TBuffer* outBuf) {
        DTokenizer.CommitDoc();
        DCreator.ProcessDirectText(BlockCreator, nullptr);
        outBuf->Swap(Block.Buffer());
    }
};

TArcTextBlockCreator::TArcTextBlockCreator() {
}

TArcTextBlockCreator::~TArcTextBlockCreator() {
}

void TArcTextBlockCreator::OpenDocument(TLangMask langs, ELanguage langprior) {
    Impl.Reset(new TImpl(langs, langprior));
}

void TArcTextBlockCreator::AddText(const wchar16* text, size_t len, const char* zoneName) {
    Y_ASSERT(!!Impl);
    Impl->AddText(text, len, zoneName);
}

void TArcTextBlockCreator::AddText(const TString& text, const char* zoneName) {
    TUtf16String seq = CharToWide(text, csYandex);
    AddText(seq.data(), text.size(), zoneName);
}

void TArcTextBlockCreator::CommitDocument(TBuffer* outBuf) {
    Y_ASSERT(!!Impl);
    Impl->CommitDocument(outBuf);
    Impl.Destroy();
}

}
