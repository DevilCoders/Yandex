#include "hilited_string.h"

namespace NUrlCutter {
    THilitedString::THilitedString() {
    }

    THilitedString::THilitedString(const TUtf16String& str)
        : String(str) {
    }

    THilitedString& THilitedString::Append(const THilitedString& hs) {
        size_t ofs = String.length();
        String.append(hs.String);

        SortedHilitedSpans.reserve(SortedHilitedSpans.size() + hs.SortedHilitedSpans.size());
        for (const auto& span : hs.SortedHilitedSpans) {
            SortedHilitedSpans.push_back({span.first + ofs, span.second + ofs});
        }
        return *this;
    }

    THilitedString& THilitedString::Append(const TUtf16String& str) {
        String.append(str);
        return *this;
    }

    TUtf16String THilitedString::Merge(const TUtf16String& openMark, const TUtf16String& closeMark) const {
        TUtf16String merged(Reserve(String.size() + (openMark.size() + closeMark.size()) * SortedHilitedSpans.size()));
        size_t ofs = 0;
        for (const auto& span : SortedHilitedSpans) {
            merged.append(String.data() + ofs, String.data() + span.first);
            merged.append(openMark);
            merged.append(String.data() + span.first, String.data() + span.second);
            merged.append(closeMark);
            ofs = span.second;
        }
        merged.append(String.data() + ofs, String.data() + String.size());
        return merged;
    }

    void THilitedString::FillHilitedWords(TVector<TUtf16String>& hlWords) const {
        hlWords.reserve(hlWords.size() + SortedHilitedSpans.size());
        for (const auto& span : SortedHilitedSpans) {
            TUtf16String hlWord(String.data() + span.first, String.data() + span.second);
            hlWords.push_back(std::move(hlWord));
        }
    }

    TVector<TUtf16String> THilitedString::GetHilitedWords() const {
        TVector<TUtf16String> hlWords;
        FillHilitedWords(hlWords);
        return hlWords;
    }

}
