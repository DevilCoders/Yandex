#include "pds.h"

#include <library/cpp/html/face/propface.h>
#include <library/cpp/html/zoneconf/ht_conf.h>
#include <library/cpp/html/zoneconf/attrextractor.h>
#include <library/cpp/html/html5/parse.h>
#include <library/cpp/numerator/numerate.h>
#include <util/stream/buffer.h>
#include <util/stream/zerocopy.h>

template <class TInp>
static void ParseHtml(TInp* input, const char* url, NHtml::TStorage* storage, TBuffer& normalizedDoc) {
    storage->SetPeerMode(NHtml::TStorage::ExternalText);
    normalizedDoc.Clear();
    {
        TBufferOutput out(normalizedDoc);
        TransferData(input, &out);
    }
    NHtml::TParserResult parserResult(*storage);
    NHtml5::ParseHtml(normalizedDoc, &parserResult, url);
}

TSimpleParsedDocStorage::TSimpleParsedDocStorage()
    : Storage()
{
}

TSimpleParsedDocStorage::~TSimpleParsedDocStorage() {
}

void TSimpleParsedDocStorage::DoParseHtml(IZeroCopyInput* input, const char* url) {
    ParseHtml(input, url, &Storage, NormalizedDoc);
}

void TSimpleParsedDocStorage::DoParseHtml(IInputStream* input, const char* url) {
    ParseHtml(input, url, &Storage, NormalizedDoc);
}

void TSimpleParsedDocStorage::DoParseHtml(const char* doc, size_t sz, const char* url) {
    TMemoryInput input(doc, sz);
    DoParseHtml(&input, url);
}

bool TSimpleParsedDocStorage::DoNumerateHtml(INumeratorHandler& handler, IParsedDocProperties* docProps, IZoneAttrConf* config) {
    return Numerate(handler, Storage.Begin(), Storage.End(), docProps, config);
}

void TSimpleParsedDocStorage::DoSomething(IParserResult* res) {
    NHtml::TSegmentedQueueIterator first = Storage.Begin();
    NHtml::TSegmentedQueueIterator last = Storage.End();
    while (first != last) {
        const THtmlChunk* const ev = GetHtmlChunk(first);
        if (ev->flags.type <= (int)PARSED_EOF)
            break;
        ++first;
        res->OnHtmlChunk(*ev);
    }
}

THtProcessor::THtProcessor() {
    Configurator.Reset(new THtConfigurator);
    Configurator->LoadDefaultConf();
}

THtProcessor::~THtProcessor() {
}

void THtProcessor::Configure(const char* filename) {
    Configurator->Configure(filename);
}

THolder<IParsedDocProperties> THtProcessor::ParseHtml(const char* doc, size_t sz, const char* url) {
    DocStorage.DoParseHtml(doc, sz, url);
    return MakeProperties(url);
}

THolder<IParsedDocProperties> THtProcessor::ParseHtml(IZeroCopyInput* input, const char* url) {
    DocStorage.DoParseHtml(input, url);
    return MakeProperties(url);
}

THolder<IParsedDocProperties> THtProcessor::ParseHtml(IInputStream* input, const char* url) {
    DocStorage.DoParseHtml(input, url);
    return MakeProperties(url);
}

THolder<IParsedDocProperties> THtProcessor::MakeProperties(const char* url) const {
    THolder<IParsedDocProperties> docProps(CreateParsedDocProperties());
    if (url && *url) {
        docProps->SetProperty(PP_BASE, url);
    }
    return docProps;
}

bool THtProcessor::NumerateHtml(INumeratorHandler& handler, IParsedDocProperties* docProps) {
    TAttributeExtractor extractor(Configurator.Get());
    return DocStorage.DoNumerateHtml(handler, docProps, &extractor);
}

bool NumerateHtmlSimple(INumeratorHandler& handler,
                        const char* doc,
                        size_t sz,
                        const char* url /*= NULL*/,
                        const char* enc /*= "utf-8"*/,
                        const char* cfg /*= NULL*/) {
    THtProcessor htProcessor;
    if (cfg && *cfg)
        htProcessor.Configure(cfg);
    THolder<IParsedDocProperties> docProps(htProcessor.ParseHtml(doc, sz, url));
    if (enc && *enc)
        docProps->SetProperty(PP_CHARSET, enc);
    return htProcessor.NumerateHtml(handler, docProps.Get());
}

bool NumerateHtmlSimple(INumeratorHandler& handler,
                        THtProcessor& htProcessor,
                        const char* doc,
                        size_t sz,
                        const char* url /*= NULL*/,
                        const char* enc /*= "utf-8"*/) {
    htProcessor.Clear();
    THolder<IParsedDocProperties> docProps(htProcessor.ParseHtml(doc, sz, url));
    if ((enc != nullptr) && (enc[0] != 0))
        docProps->SetProperty(PP_CHARSET, enc);
    return htProcessor.NumerateHtml(handler, docProps.Get());
}
