#pragma once

#include <util/generic/buffer.h>
#include <util/generic/ptr.h>
#include <library/cpp/charset/doccodes.h>

#include <library/cpp/html/storage/storage.h>

class INumeratorHandler;
class IParsedDocProperties;
class IZeroCopyInput;
class IInputStream;
class IZoneAttrConf;
class THtConfigurator;

class TSimpleParsedDocStorage: private TNonCopyable {
public:
    TSimpleParsedDocStorage();
    ~TSimpleParsedDocStorage();

public:
    void DoParseHtml(const char* doc, size_t sz, const char* url);
    void DoParseHtml(IZeroCopyInput* input, const char* url);
    void DoParseHtml(IInputStream* input, const char* url);
    bool DoNumerateHtml(INumeratorHandler& handler, IParsedDocProperties* docProps, IZoneAttrConf* config);
    void DoSomething(IParserResult* res);

    const NHtml::TSegmentedQueueIterator Begin() const {
        return Storage.Begin();
    }
    const NHtml::TSegmentedQueueIterator End() const {
        return Storage.End();
    }
    void Clear() {
        Storage.Clear();
    }
    TStringBuf GetParsedDoc() const {
        return TStringBuf(NormalizedDoc.Data(), NormalizedDoc.Size());
    }

protected:
    NHtml::TStorage Storage;

private:
    TBuffer NormalizedDoc;
};

class THtProcessor {
public:
    THtProcessor();
    ~THtProcessor();

    void Configure(const char* filename);

    THolder<IParsedDocProperties> ParseHtml(const char* doc, size_t sz, const char* url = nullptr);
    THolder<IParsedDocProperties> ParseHtml(IZeroCopyInput* input, const char* url = nullptr);
    THolder<IParsedDocProperties> ParseHtml(IInputStream* input, const char* url = nullptr);
    bool NumerateHtml(INumeratorHandler& handler, IParsedDocProperties* docProps);

    const TSimpleParsedDocStorage& GetStorage() const {
        return DocStorage;
    }

    void Clear() {
        DocStorage.Clear();
    }

private:
    THolder<IParsedDocProperties> MakeProperties(const char* url) const;

private:
    TSimpleParsedDocStorage DocStorage;
    THolder<THtConfigurator> Configurator;
};

bool NumerateHtmlSimple(INumeratorHandler& handler,
                        const char* doc,
                        size_t sz,
                        const char* url = nullptr,
                        const char* enc = "utf-8",
                        const char* cfg = nullptr);

bool NumerateHtmlSimple(INumeratorHandler& handler,
                        THtProcessor& htProcessor,
                        const char* doc,
                        size_t sz,
                        const char* url = nullptr,
                        const char* enc = "utf-8");
