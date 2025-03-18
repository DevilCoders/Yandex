#include "metricsserp.h"

#include <tools/snipmake/serpmetrics_xml_parser/metricsserp.xsyn.h>

#include <library/cpp/logger/priority.h>
#include <library/cpp/xml/parslib/xmlsax.h>

#include <util/charset/wide.h>
#include <util/generic/deque.h>
#include <util/string/cast.h>

namespace NSnippets {

    class TMetricsSerp {
    private:
        bool ValidDocType;
        TString SnippetText;
        THolder<TReqSerp> CurrentSerp;
        TReqSnip* CurrentDoc;
        size_t SnipIndex, SerpIndex;
    protected:
        void SetQueryText(const char* text, size_t len);
        void SetRegion(const char* value);
        void SetDocumentType(const char* value);
        void SetWizardType(const char* value);

        void SerpBegin();
        void SerpEnd();

        void DocumentBegin();
        void DocumentEnd();

        void SetTitle(const char* text, size_t len);
        void SetSnippet(const char* text, size_t len);
        void SetPageUrl(const char* text, size_t len);

        void SitelinkBegin();
        void SetSitelinkUrl(const char* text, size_t len);
        void SetSitelinkTitle(const char* text, size_t len);
        void SetSitelinkSnippet(const char* text, size_t len);

        void JudgementBegin();
        void SetLabel(const char* value);

        void ErrMessage(ELogPriority /*priority*/, const char* fmt, ...);
    public:
        TMetricsSerp();
        virtual ~TMetricsSerp();
        typedef TSimpleSharedPtr<TReqSerp> TReqSerpPtr;
        TDeque<TReqSerpPtr> Serps;
    };

    TMetricsSerp::TMetricsSerp()
        : ValidDocType(false)
        , SnipIndex(0)
        , SerpIndex(0)
    {}

    TMetricsSerp::~TMetricsSerp(){}

    void TMetricsSerp::SetQueryText(const char* text, size_t len) {
        if (CurrentSerp.Get())
            CurrentSerp->Query.append(text, len);
        SnipIndex = 1;
    }

    void TMetricsSerp::SetRegion(const char* value) {
        if (CurrentSerp.Get())
            CurrentSerp->Region = value;
    }

    void TMetricsSerp::SetDocumentType(const char* value) {
        ValidDocType = (strcmp(value,"SEARCH_RESULT") == 0);
    }

    void TMetricsSerp::SetWizardType(const char* value) {
        if (ValidDocType) {
            CurrentDoc->WizardType = value;
        }
    }

    void TMetricsSerp::SerpBegin() {
        CurrentSerp.Reset(new TReqSerp);
        CurrentSerp->Id = SerpIndex;
        ++SerpIndex;
    }

    void TMetricsSerp::SerpEnd() {
        Serps.push_back(CurrentSerp.Release());
    }

    void TMetricsSerp::DocumentBegin() {
        if (CurrentSerp.Get()) {
            CurrentSerp->Snippets.emplace_back();
            CurrentDoc = &CurrentSerp->Snippets.back();
            CurrentDoc->Query = CurrentSerp->Query;
            CurrentDoc->Region = CurrentSerp->Region;
        }
        SnippetText.clear();
        ValidDocType = true;
    }

    void TMetricsSerp::DocumentEnd(){
        if (ValidDocType) {
            if (!SnippetText.empty())
                CurrentDoc->SnipText.push_back(TSnipFragment(UTF8ToWide(SnippetText)));
            CurrentDoc->Id = ToString(SnipIndex++);
            CurrentDoc = nullptr;
        }
    }

    void TMetricsSerp::SetTitle(const char* text, size_t len) {
        if (ValidDocType)
            CurrentDoc->TitleText += UTF8ToWide(text,len);
    }

    void TMetricsSerp::SetSnippet(const char* text, size_t len) {
        if (ValidDocType)
            SnippetText.append(text,len);
    }

    void TMetricsSerp::SetPageUrl(const char* text, size_t len) {
        if (ValidDocType)
            CurrentDoc->Url += UTF8ToWide(text, len);
    }

    void TMetricsSerp::SitelinkBegin() {
        if (ValidDocType) {
            CurrentDoc->Sitelinks.emplace_back();
        }
    }

    void TMetricsSerp::SetSitelinkUrl(const char* text, size_t len) {
        if (ValidDocType)
            CurrentDoc->Sitelinks.back().Url += UTF8ToWide(text, len);
    }

    void TMetricsSerp::SetSitelinkTitle(const char* text, size_t len) {
        if (ValidDocType)
            CurrentDoc->Sitelinks.back().Title += UTF8ToWide(text, len);
    }

    void TMetricsSerp::SetSitelinkSnippet(const char* text, size_t len) {
        if (ValidDocType) {
            CurrentDoc->Sitelinks.back().Snippet += UTF8ToWide(text, len);
            CurrentDoc->IsBNA = true;
            CurrentSerp->HasBNA = true;
        }
    }

    void TMetricsSerp::JudgementBegin() {
        if (ValidDocType) {
            CurrentSerp->Snippets.back().Marks.push_back(TSnipMark());
        }
    }

    void TMetricsSerp::SetLabel(const char* value) {
        if (ValidDocType) {
            CurrentSerp->Snippets.back().Marks.back().Label = value;
        }
    }

    void TMetricsSerp::ErrMessage(ELogPriority /*priority*/, const char* fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        vprintf(fmt, ap);
        va_end(ap);
    }

    struct TSerpXmlReader::TParser {
        typedef TXmlSaxParser< TMetricsSerpParser<TMetricsSerp> > TSerpXmlParser;
        TSerpXmlParser Impl;
    };

    TSerpXmlReader::TSerpXmlReader(IInputStream* serp)
      : Data(serp, ZLib::Auto)    // default buffer size
      , Parser(new TParser())
      , Eof(false)
    {
        Parser->Impl.Start(true);
    }

    TSerpXmlReader::~TSerpXmlReader() {
    }

    bool TSerpXmlReader::Next() {
        if (!Parser->Impl.Serps.empty()) { // drop previous
            Parser->Impl.Serps.pop_front();
        }
        while (!Eof && Parser->Impl.Serps.size() < 1) {
            const char* ptr;
            size_t len = Data.Next(&ptr);
            if (len) {
                Parser->Impl.Parse(ptr, len);
            } else {
                Eof = true;
                break; // ok, eof is fine too
            }
        }
        return !Parser->Impl.Serps.empty();
    }

    const TReqSerp& TSerpXmlReader::Get() const {
        return *Parser->Impl.Serps[0].Get();
    }
};
