#pragma once

#include <util/generic/array_ref.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>

#include <array>

//*********************** search routines ***************************

void CutPassage(TString &Passage, unsigned CutTo, bool InsLeadingTrailingDots = false); ///< for compatibility w/ XML report
void CutPassage(TUtf16String& passage, unsigned cutTo, bool insLeadingTrailingDots = false);
void CutPassage(TUtf16String& Passage, unsigned CutTo, bool AccountForEllipsisLen, const TUtf16String& LeadEllipsis, const TUtf16String& TailEllipsis);
void ReplaceHilites(TUtf16String& ToReplace, bool WithContent, const TArrayRef<const char*> HtmlHilites = TArrayRef<const char*>());
void ReplaceHilites(TString& toReplace, bool replaceOrDelete, const TArrayRef<const char*> htmlHilites = TArrayRef<const char*>());
void GlueSentences(const TVector<TUtf16String>& sentences, TUtf16String& result, bool insert3Dots = false);

TString
ConvertRawText(const char* rawtext, ui32 maxLen, int mode,
                   const char *open1, const char *open2, const char *open3,
                   const char *close1, const char *close2, const char *close3);

TString
ConvertRawTextBuf(const TStringBuf rawtext, ui32 maxLen, int mode,
                   const char *Open1, const char *Open2, const char *Open3,
                   const char *Close1, const char *Close2, const char *Close3);

const int    NUM_MARKS = 4;
const char   HILIGHT_MARK = 0x07;
const int    MARK_LEN = 2;
const char   HILIGHTS[NUM_MARKS * 2 + 1 + 1 + 1] = "([{+)]}-%#";
static constexpr std::array<TStringBuf, NUM_MARKS * 2 + 1 + 1> DEF_HTML_HILIGHTS = {
                TStringBuf("<b><font color = #800000>"),
                TStringBuf("<b><font color = #a00000>"),
                TStringBuf("<b><font color = #c00000>"),
                TStringBuf("<b><font color = #0000ff>"),
                TStringBuf("</font></b>"),
                TStringBuf("</font></b>"),
                TStringBuf("</font></b>"),
                TStringBuf("</font></b>"),
                TStringBuf("<br>"),
                TStringBuf("")};

const char TITLE_MARK = '#';
