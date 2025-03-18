#include "serp_xml_reader.h"

#include <tools/snipmake/steam/serp_parser/xmlserps.xsyn.h>

#include <library/cpp/logger/priority.h>
#include <library/cpp/xml/parslib/xmlsax.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/input.h>

namespace NSnippets
{
    class TXmlSerps {
    private:
        TReqSnip CurrentSnip;
        TReqSnip CurrentQuery;
        TString CurrentType;

        TVector<TReqSnip> Data;
        size_t SnipIndex;
    public:


        TXmlSerps()
            : SnipIndex(0)
        {
        }

        inline size_t Size() const
        {
            return Data.size();
        }

        inline const TReqSnip& Get(size_t index) const
        {
            return Data[index];
        }

        inline void Clear()
        {
            Data.clear();
        }

    protected:
        void StartQuery();
        void StartResult();
        void EndQuery();
        void EndResult();

        void SetRegion(const char* value);
        void SetType(const char* value);
        void SetUrl(const char* text, size_t len);
        void SetQueryText(const char* text, size_t len);
        void SetFragment(const char* text, size_t len);
        void SetTitleText(const char* text, size_t len);

        void ErrMessage(ELogPriority /*priority*/, const char* fmt, ...);
    };

    struct TSerpsParser {
        TXmlSaxParser<TSerpsXmlParser<TXmlSerps> > Impl;
    };


    // TXmlSerps start
    void TXmlSerps::StartQuery()
    {
        CurrentQuery = TReqSnip();
    }

    void TXmlSerps::StartResult()
    {
        CurrentSnip = CurrentQuery;
        CurrentSnip.Id = ToString(++SnipIndex);
        CurrentSnip.SnipText.push_back(TSnipFragment());
    }

    void TXmlSerps::EndResult()
    {
        if (CurrentType == "SEARCH_RESULT") {
            Data.push_back(CurrentSnip);
        }
    }

    void TXmlSerps::EndQuery()
    {
    }

    void TXmlSerps::SetRegion(const char* value)
    {
        CurrentQuery.Region = value;
    }

    void TXmlSerps::SetType(const char* value)
    {
        CurrentType = value;
    }

    void TXmlSerps::SetUrl(const char* text, size_t len)
    {
        CurrentSnip.Url += UTF8ToWide(text, len);
    }

    void TXmlSerps::SetQueryText(const char* text, size_t len)
    {
        CurrentQuery.Query += TString(text, len);
    }

    void TXmlSerps::SetTitleText(const char* text, size_t len)
    {
        CurrentSnip.TitleText += UTF8ToWide(text, len);
    }

    void TXmlSerps::SetFragment(const char* text, size_t len)
    {
        CurrentSnip.SnipText.back().Text += UTF8ToWide(text, len);
    }

    void TXmlSerps::ErrMessage(ELogPriority /*priority*/, const char* fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        vprintf(fmt, ap);
        va_end(ap);
    }
    // TXmlSerps end

    // TSerpsXmlIterator start
    TSerpsXmlIterator::TSerpsXmlIterator(IInputStream* inp)
        : Input(inp)
        , Parser(new TSerpsParser())
        , Index(0)
    {
        Parser->Impl.Start(true);
    }

    bool TSerpsXmlIterator::Next()
    {
        if (++Index >= Parser->Impl.Size()) {
            Index = 0;
            Parser->Impl.Clear();

            const size_t bufSize = 1024 * 100;
            char buf[bufSize];

            size_t sz = 1;

            while (Parser->Impl.Size() == 0 && sz) {
                sz = Input->Read(buf, bufSize);
                Parser->Impl.Parse(buf, sz);
            }
        }
        return Parser->Impl.Size() > 0;
    }

    const TReqSnip& TSerpsXmlIterator::Get() const
    {
        return Parser->Impl.Get(Index);
    }

    TSerpsXmlIterator::~TSerpsXmlIterator()
    {
        try {
            Parser->Impl.Final();
        } catch (...) {
            Cerr << "Exception caught in TSerpsXmlIterator dtor" << Endl;
        }
    }
    // TSerpsXmlIterator end
}
