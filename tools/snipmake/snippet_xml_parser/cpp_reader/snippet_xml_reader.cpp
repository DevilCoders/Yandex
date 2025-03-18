#include "snippet_xml_reader.h"
#include <util/stream/input.h>
#include <kernel/qtree/richrequest/richnode.h>
#include <tools/snipmake/snippet_xml_parser/cpp_reader/xmlsnippets.xsyn.h>

namespace NSnippets
{
    class TXmlSnippets {
    private:
        TReqSnip CurrentQD;
        TReqSnip CurrentSnip;
        TSnipMark CurrentMark;

        int FragIdx;
        int FragCnt;
        TVector<TReqSnip> Data;
        size_t SnipIndex;
        bool DeserializeQTree;
    public:


        TXmlSnippets()
            : FragIdx(0)
            , FragCnt(0)
            , SnipIndex(0)
            , DeserializeQTree(true)
        {
        }

        void SetDeserializeQTree(bool value) {
            DeserializeQTree = value;
        }

        inline size_t Size()
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
        void StartQDPair();
        void StartSnippet();
        void StartMark();
        void EndMark();
        void EndSnippet();
        void EndQDPair();

        void SetRegion(const char* value);
        void SetB64QueryTree(const char* value);
        void SetUrl(const char* value);
        void SetRelevance(const char* value);
        void SetQueryText(const char* text, size_t len);
        void SetAlgorithm(const char* value);
        void SetFragmentsCount(const char* value);
        void SetLines(const char* value);
        void SetRank(const char* value);
        void SetTitle(const char* text, size_t len);
        void SetFragmentCoords(const char* value);
        void SetFragmentArcCoords(const char* value);
        void AddFragment(const char* text, size_t len);
        void StartFragment();
        void EndFragment();
        void SetFeaturesString(const char* text, size_t len);
        void SetValue(const char* value);
        void SetCriteria(const char* value);
        void SetAssessor(const char* value);
        void SetQuality(const char* value);
        void SetTimestamp(const char* value);


        void ErrMessage(ELogPriority /*priority*/, const char* fmt, ...);
    };

    struct TSnippetsParser { //Crap++
        TXmlSaxParser<TSnippetsXmlParser<TXmlSnippets> > Impl;
    };

    // TXmlSnippets start
    void TXmlSnippets::StartQDPair()
    {
        CurrentQD = TReqSnip();
    }

    void TXmlSnippets::StartSnippet()
    {
        CurrentSnip = CurrentQD;
        CurrentSnip.Id = ToString(++SnipIndex);
        FragCnt = 0;
        FragIdx = 0;
    }

    void TXmlSnippets::StartMark() {
        CurrentMark.Value = 0;
        CurrentMark.Criteria.clear();
        CurrentMark.Assessor.clear();
        CurrentMark.Quality = 0.0f;
        CurrentMark.Timestamp.clear();
    }
    void TXmlSnippets::EndMark() {
        CurrentSnip.Marks.push_back(CurrentMark);
    }

    void TXmlSnippets::EndSnippet()
    {
        Y_ASSERT(!FragCnt || FragCnt == FragIdx);
        if (DeserializeQTree && !CurrentQD.RichRequestTree) {
            try {
                CurrentQD.RichRequestTree = DeserializeRichTree(DecodeRichTreeBase64(CurrentQD.B64QTree));
            } catch (...) {
                Cerr << "Broken qtree: " << CurrentQD.B64QTree << Endl;
                return;
            }
            CurrentSnip.RichRequestTree = CurrentQD.RichRequestTree;
        }

        Data.push_back(CurrentSnip);
    }

    void TXmlSnippets::EndQDPair()
    {
    }

    void TXmlSnippets::SetRegion(const char* value)
    {
        CurrentQD.Region = value;
    }

    void TXmlSnippets::SetB64QueryTree(const char* value)
    {
        CurrentQD.B64QTree = value;
    }

    void TXmlSnippets::SetUrl(const char* value)
    {
        CurrentQD.Url = UTF8ToWide(value);
    }

    void TXmlSnippets::SetRelevance(const char* value)
    {
        CurrentQD.Relevance = value;
    }

    void TXmlSnippets::SetQueryText(const char* text, size_t len)
    {
        CurrentQD.Query += TString(text, len);
    }

    void TXmlSnippets::SetAlgorithm(const char* value)
    {
        CurrentSnip.Algo = value;
    }

    void TXmlSnippets::SetFragmentsCount(const char* value)
    {
        FragCnt = FromString<int>(value);
    }

    void TXmlSnippets::SetLines(const char* value)
    {
        CurrentSnip.Lines = value;
    }

    void TXmlSnippets::SetRank(const char* value)
    {
        CurrentSnip.Rank = value;
    }

    void TXmlSnippets::SetTitle(const char* text, size_t len)
    {
        CurrentSnip.TitleText += UTF8ToWide(text, len);
    }

    void TXmlSnippets::SetFragmentCoords(const char* value)
    {
        CurrentSnip.SnipText.back().Coords = value;
    }

    void TXmlSnippets::SetFragmentArcCoords(const char* value)
    {
        CurrentSnip.SnipText.back().ArcCoords = value;
    }

    void TXmlSnippets::AddFragment(const char* text, size_t len)
    {
        CurrentSnip.SnipText.back().Text += UTF8ToWide(text, len);
    }

    void TXmlSnippets::StartFragment()
    {
        CurrentSnip.SnipText.push_back(TSnipFragment());
    }

    void TXmlSnippets::EndFragment()
    {
        ++FragIdx;
    }

    void TXmlSnippets::SetFeaturesString(const char* text, size_t len)
    {
        CurrentSnip.FeatureString += TString(text, len);
    }

    void TXmlSnippets::SetValue(const char* value) {
        CurrentMark.Value = FromString<int>(value);
    }
    void TXmlSnippets::SetCriteria(const char* value) {
        CurrentMark.Criteria = UTF8ToWide(value);
    }
    void TXmlSnippets::SetAssessor(const char* value) {
        CurrentMark.Assessor = UTF8ToWide(value);
    }
    void TXmlSnippets::SetQuality(const char* value) {
        CurrentMark.Quality = FromString<float>(value);
    }
    void TXmlSnippets::SetTimestamp(const char* value) {
        CurrentMark.Timestamp = UTF8ToWide(value);
    }

    void TXmlSnippets::ErrMessage(ELogPriority /*priority*/, const char* fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        vprintf(fmt, ap);
        va_end(ap);
    }
    // TXmlSnippets end

    // TSnippetsXmlIterator start
    TSnippetsXmlIterator::TSnippetsXmlIterator(IInputStream* inp, bool deserializeQTree)
        : Input(inp)
        , Parser(new TSnippetsParser())
        , Index(0)
    {
        Parser->Impl.SetDeserializeQTree(deserializeQTree);
        Parser->Impl.Start(true);
    }

    bool TSnippetsXmlIterator::Next()
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

    const TReqSnip& TSnippetsXmlIterator::Get() const
    {
        return Parser->Impl.Get(Index);
    }

    TSnippetsXmlIterator::~TSnippetsXmlIterator()
    {
        try {
            Parser->Impl.Final();
        } catch (...) {
            Cerr << "Exception caught in TSnippetsXmlIterator dtor" << Endl;
        }
    }
    // TSnippetsXmlIterator end
}
