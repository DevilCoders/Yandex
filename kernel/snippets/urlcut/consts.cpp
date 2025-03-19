#include "consts.h"

#include <util/generic/strbuf.h>
#include <util/string/subst.h>

namespace NUrlCutter {
    static constexpr TStringBuf GLAY_LIST[] = {
        TStringBuf("archive"),
        TStringBuf("article"),
        TStringBuf("articles"),
        TStringBuf("artist"),
        TStringBuf("contact"),
        TStringBuf("contacts"),
        TStringBuf("download"),
        TStringBuf("downloads"),
        TStringBuf("forum"),
        TStringBuf("music"),
        TStringBuf("news"),
        TStringBuf("price"),
        TStringBuf("soft"),
        TStringBuf("video"),
    };
    static THashMap<TUtf16String, i32> InitGrayList() {
        THashMap<TUtf16String, i32> gl;
        for (const TStringBuf& word : GLAY_LIST) {
            gl[ASCIIToWide(word)] = MAX_W;
        }
        return gl;
    }
    const THashMap<TUtf16String, i32> GrayList(InitGrayList());

    static constexpr TStringBuf STOP_WORDS[] = {
        TStringBuf("http"),
        TStringBuf("www"),
        TStringBuf("html"),
        TStringBuf("php"),
        TStringBuf("index"),
        TStringBuf("htm"),
        TStringBuf("news"),
        TStringBuf("forum"),
        TStringBuf("page"),
        TStringBuf("info"),
        TStringBuf("asp"),
        TStringBuf("print"),
        TStringBuf("catalogue"),
        TStringBuf("all"),
        TStringBuf("shtml"),
        TStringBuf("cgi"),
        TStringBuf("view"),
        TStringBuf("com"),
        TStringBuf("org"),
        TStringBuf("net"),
        TStringBuf("ru"),
        TStringBuf("tr"),
        TStringBuf("kz"),
        TStringBuf("by"),
        TStringBuf("ua"),
    };
    static THashSet<TUtf16String> InitStopWords() {
        THashSet<TUtf16String> sw;
        for (const TStringBuf& word : STOP_WORDS) {
            sw.insert(ASCIIToWide(word));
        }
        return sw;
    }
    const THashSet<TUtf16String> StopWords(InitStopWords());

}
