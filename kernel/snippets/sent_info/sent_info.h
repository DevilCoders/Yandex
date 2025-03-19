#pragma once

#include <kernel/snippets/archive/view/view.h>
#include <kernel/snippets/config/enums.h>

#include <library/cpp/token/nlptypes.h>

#include <util/generic/noncopyable.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/hash_set.h>

class TArchiveMarkupZones;

namespace NSnippets {
    namespace NSegments {
        class TSegmentsInfo;
    }
    class TSentWord;
    class TSentMultiword;
    class TArchiveMarkup;
    class TArchiveSent;

    class TSentsInfo: private TNonCopyable {
    public:
        struct TWordPodStroka {
            unsigned int Ofs = 0;
            unsigned int Len = 0;
            size_t Hash = 0;
            size_t N = 0;
            size_t EndOfs() const {
                return Ofs + Len;
            }
        };
        struct TWordVal {
            TWordPodStroka Word;
            bool IsSuffix = false;
            int SentId = 0;
            TWtringBuf Origin;
            unsigned int TextBufBegin = 0;
            unsigned int TextBufEnd = 0;
            NLP_TYPE Type;
        };
        struct TSentPodStroka {
            unsigned int Ofs = 0;
            unsigned int Len = 0;
            size_t EndOfs() const {
                return Ofs + Len;
            }
        };
        struct TSentVal {
            TSentPodStroka Sent;
            int StartInWords = 0;
            int LengthInWords = 0;
            int OffsetInPara = 0;
            int ParaLenInSents = 0;
            int ParaLenInWords = 0;
            const TArchiveSent* ArchiveSent = nullptr;
            TWtringBuf ArchiveOrigin;
            ESentsSourceType SourceType = SST_TEXT;
        };
        TVector<TWordVal> WordVal;
        TVector<TSentVal> SentVal;
        TUtf16String Text;
        size_t MaxN = 0;
        THashMap<size_t, size_t> W2H;
        const TArchiveView MetaDescrAdd;

    private:
        THashMap<int, std::pair<int, int>> TextArchIdToSents;
        const NSegments::TSegmentsInfo* Segments = nullptr;
        const TArchiveMarkupZones& TextArcMarkupZones;

    public:
        TSentsInfo(
            const TArchiveMarkup* markup,
            const TArchiveView& vText,
            const TArchiveView* metaDescrAdd,
            bool putDot,
            bool paraTables);

    private:
        void Init(const TArchiveView& vText,
                  bool putDot,
                  bool paraTables);

    public:
        template <class T>
        T Begin() const;
        template <class T>
        T End() const;

        int WordCount() const;
        int SentencesCount() const;
        std::pair<int, int> TextArchIdToSentIds(int archId) const;
        TSentWord WordId2SentWord(int wordId) const;
        int GetOrigSentId(int i) const;
        int WordId2SentId(int wordId) const;
        int FirstWordIdInSent(int sentId) const;
        int LastWordIdInSent(int sentId) const;
        int GetSentLengthInWords(int sentId) const;
        const TArchiveSent& GetArchiveSent(int sentId) const;
        bool IsSentIdFirstInArchiveSent(int sentId) const;
        bool IsSentIdFirstInPara(int sentId) const;
        bool IsSentIdLastInPara(int sentId) const;
        bool IsWordIdFirstInSent(int wordId) const;
        bool IsWordIdLastInSent(int wordId) const;
        bool IsCharIdFirstInWord(size_t charId, int wordId) const;
        void OutWords(int i, int j) const;
        const NSegments::TSegmentsInfo* GetSegments() const;
        const TArchiveMarkupZones& GetTextArcMarkup() const;

        // If Text = "«Foo, bar» - baz. Qux! "
        //  GetTextBuf(0, 1) == "«Foo, bar»"
        //  GetTextWithEllipsis(0, 1) == "«Foo, bar»..."
        //  GetWordBuf(0) == "Foo"
        //  GetBlanksBefore(3) == ". "
        //  GetBlanksAfter(1) == "» - "
        //  GetSentBeginBlanks(1) == ""
        //  GetSentEndBlanks(1) == "! "
        //  GetWordSpanBuf(0, 1) == "Foo, bar"
        //  GetWordSentSpanBuf(0, 2) == "«Foo, bar» - baz. "
        //  GetSentBuf(0) == "«Foo, bar» - baz. "
        //  GetSentSpanBuf(0, 1) == "«Foo, bar» - baz. Qux! "

        TWtringBuf GetTextBuf(int i, int j) const;
        TUtf16String GetTextWithEllipsis(int i, int j) const;
        void GetTextWithEllipsis(int i, int j, TWtringBuf& prefix, TWtringBuf& text, TWtringBuf& suffix) const;
        NLP_TYPE GetWordType(int wordId) const;
        TWtringBuf GetWordBuf(int wordId) const;
        TWtringBuf GetBlanksBefore(int wordId) const;
        TWtringBuf GetBlanksAfter(int wordId) const;
        TWtringBuf GetSentBeginBlanks(int sentId) const;
        TWtringBuf GetSentEndBlanks(int sentId) const;
        TWtringBuf GetWordSpanBuf(int w0, int w1) const;
        TWtringBuf GetWordSentSpanBuf(int w0, int w1) const;
        TWtringBuf GetSentBuf(int sentId) const;
        TWtringBuf GetSentSpanBuf(int s0, int s1) const;
    };

    template <>
    TSentWord TSentsInfo::Begin() const;
    template <>
    TSentMultiword TSentsInfo::Begin() const;
    template <>
    TSentWord TSentsInfo::End() const;
    template <>
    TSentMultiword TSentsInfo::End() const;
}
