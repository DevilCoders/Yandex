#pragma once

#include <kernel/snippets/strhl/zonedstring.h>

#include <library/cpp/langs/langs.h>
#include <util/generic/vector.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <utility>

namespace NSnippets {

class TSentsMatchInfo;
class TSingleSnip;
class TConfig;
struct ISnippetDebugOutputHandler;

class TGluer {
public:
    struct TImpl;
private:
    THolder<TImpl> Impl;
public:
    TGluer(const TSingleSnip* ss, const THiliteMark* ellipsis);
    ~TGluer();

    void MarkMatches(const THiliteMark* mark);
    void MarkMatchedPhones(const THiliteMark* mark);
    void MarkAllPhones(const THiliteMark* mark);
    void MarkExt(const TVector<std::pair<int, int>>& v, const THiliteMark* mark);
    void MarkParabeg(const THiliteMark* mark);
    void MarkPunctTrash(const THiliteMark* mark);
    void MarkFio(const THiliteMark* mark);
    void MarkSentences(const THiliteMark* mark);
    void MarkMenuWords(const THiliteMark* mark);
    // extSpans contains already built spans, but from all fragments, not from the current only. So filtering is needed
    void MarkLinks(const TVector<TZonedString::TSpan>& linkSpans, const THiliteMark* mark);
    void MarkTableCells(const THiliteMark* mark);

    TZonedString GlueToZonedString() const;

    static TZonedString EmbedPara(const TZonedString& z);
    static TZonedString EmbedExtMarks(const TZonedString& z);
    static TZonedString EmbedTableCellMarks(const TZonedString& z);
    static TZonedString EmbedLinkMarks(const TZonedString& z, TVector<TString>& links, const TString& docUrl);
    static TZonedString CutTrash(const TZonedString& z);
    static TZonedString Decapitalize(const TZonedString& z, ELanguage lang,
                                     ISnippetDebugOutputHandler* debug);

    static TUtf16String GlueToString(const TZonedString& z);
    static TString GlueToHtmlEscapedUTF8String(const TZonedString& z);

    static const THiliteMark ParaMark;
};

TUtf16String GlueTitle(const TVector<TUtf16String>& titles);
TUtf16String GlueHeadline(const TVector<TUtf16String>& headlines);

// fixes non-breaking space and RTL-override for now
void FixWeirdChars(TUtf16String& s);

}
