#pragma once
#include "hilite_mark.h"
#include <util/generic/ptr.h>
#include <util/generic/hash.h>
#include <util/charset/wide.h>

struct TWideToken;
struct THiliteMark;
struct TZonedString;
class TRichRequestNode;

struct TPaintingOptions {
    bool SrcOutput = false;
    bool TrustPunct = false;
    bool PaintClosePositionsSmart = false;
    bool PaintAbbrevSmart = false;
    bool PaintFios = false;
    bool SmartUnpaintFios = false;
    bool PaintDom2Smart = false;
    bool HackStopwords = false;
    bool TokHl = true;
    bool UseFioZones = false;
    bool UseFioZones2 = false;
    bool WholeTokens = false;
    bool GlueTok = false;
    bool GlueTokHack = false;
    bool SkipAttrs = false;
    bool Fred = false;

    inline static constexpr TPaintingOptions DefaultSnippetOptions() {
        TPaintingOptions res;
        res.PaintClosePositionsSmart = true; //Issue: SNIPPETS-619
        res.PaintAbbrevSmart = true; //Issue: SNIPPETS-212
        res.PaintFios = true; //Issue: SNIPPETS-635
        res.SmartUnpaintFios = true; //Issue: SNIPPETS-635
        res.PaintDom2Smart = true; //Issue: SNIPPETS-198
        res.HackStopwords = true; //Issue: SNIPPETS-912
        res.TokHl = true; //Issue: SNIPPETS-817
        res.UseFioZones = false; //Issue: SNIPPETS-635
        res.UseFioZones2 = true; //Issue: SNIPPETS-635
        res.WholeTokens = false; //Issue: SNIPPETS-1259
        res.GlueTok = true; //Issue: SNIPPETS-1260
        res.GlueTokHack = true; //Issue: SNIPPETS-1260
        return res;
    }
};

inline static constexpr TPaintingOptions DEFAULT_SNIPPET_OPTIONS = TPaintingOptions::DefaultSnippetOptions();

class TInlineHighlighter : TNonCopyable,
                public TAtomicRefCount<TInlineHighlighter>
{
public:
    TInlineHighlighter();
    ~TInlineHighlighter();

public:
    struct IPainter {
        TPaintingOptions Options;

        virtual void AddJob(TUtf16String& l) = 0;
        virtual void AddJob(TZonedString& l) = 0;
        virtual void Paint() = 0;
        virtual void DropJobs() = 0;
        virtual ~IPainter() {
        }
    };
    typedef TAtomicSharedPtr<IPainter> TIPainterPtr;
    TIPainterPtr GetPainter() const;

public:
    void AddRequest(const TRichRequestNode& richtree, const THiliteMark* marks = nullptr, bool additional = false);

    void PaintPassages(TUtf16String& passage, const TPaintingOptions& options = TPaintingOptions()) const;
    void PaintPassages(TZonedString& passage, const TPaintingOptions& options = TPaintingOptions()) const;
    void PaintPassages(TVector<TZonedString>& passages, const TPaintingOptions& options = TPaintingOptions()) const;
public:
    class TImpl;
private:
    THolder<TImpl> Impl;
private:
    friend class TLemmCountHandler;
    bool IsGoodWord(const TWideToken& tok) const;
};

inline static TString RawHiliteStr(const TInlineHighlighter& highlighter, const TStringBuf value,
    const TPaintingOptions& paintingOptions = DEFAULT_SNIPPET_OPTIONS)
{
    TUtf16String wString = TUtf16String::FromUtf8(value);
    highlighter.PaintPassages(wString, paintingOptions);
    return WideToUTF8(wString);
}
