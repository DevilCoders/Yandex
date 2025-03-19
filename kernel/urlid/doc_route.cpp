#include "doc_route.h"

#include <util/generic/typetraits.h>
#include <util/stream/output.h>
#include <util/string/cast.h>

TDocRoute TDocRoute::FromDocId(const TStringBuf& strDocIdFull, TStringBuf* /*out*/ docHash) {
    if (strDocIdFull.StartsWith(Separator))
        ythrow yexception() << "Failed to parse DocRoute " << TString(strDocIdFull).Quote()
                            << ": an empty string is not a valid value for source, "
                            "Only numbers in range 0.." << (MaxSourceNumber - 1) << " are accepted as document sources.";

    TStringBuf strDocId = strDocIdFull;
    TDocRoute ans;
    ui8 level = 0;
    bool haveToken;
    TStringBuf hashToken = strDocId.RNextTok(Separator), thisToken;
    for (haveToken = strDocId.RNextTok(Separator, thisToken); haveToken && level < MaxLength; haveToken = strDocId.RNextTok(Separator, thisToken)) {
        TSourceType source;
        if (Y_UNLIKELY(!thisToken)) {
            ythrow yexception() << "Failed to parse DocRoute " << TString(strDocIdFull).Quote()
                                << ": an empty string is not a valid value for source, "
                                "Only numbers in range 0.." << (MaxSourceNumber - 1) << " are accepted as document sources.";
        }
        if (Y_UNLIKELY(thisToken.StartsWith('+'))) {
            ythrow yexception() << "Failed to parse DocRoute " << TString(strDocIdFull).Quote()
                                << ": '+' is not an acceptable character. Digits only, please.";
        }
        if (Y_UNLIKELY(!TryFromString(thisToken, source))) {
            ythrow yexception() << "Failed to parse DocRoute " << TString(strDocIdFull).Quote()
                                << ": the offending part is " << TString(thisToken).Quote() << ".\n"
                                "Only numbers in range 0.." << (MaxSourceNumber - 1) << " are accepted as document sources.";
        }
        ans.SetSource(level++, FromString(thisToken));
    }

    const auto ml = MaxLength; //< undefined reference to 'TDocRoute::MaxLength'
    if (haveToken)
        ythrow yexception() << "TDocRoute length (which is " << ml << ") exceeded in docId " << TString(strDocIdFull).Quote();
    while (level < MaxLength)
        ans.SetSource(level++, NoSource);

    if (docHash)
        *docHash = hashToken;

    return ans;
}

template <>
void In(IInputStream& src, TDocRoute& target) {
    TString s;
    src >> s;
    target = TDocRoute::FromDocId(s);
}

TString TDocRoute::ToDocId() const {
    ui8 length = Length();
    if (length == 0)
        return "";
    TString ans = ToString(Source(--length));
    const auto sep = Separator; //< ToString(char) fails on 'static constexpr'
    while (length --> 0) {
        ans += sep;
        ans += ToString(Source(length));
    }
    return ans;
}

template <>
void Out<TDocRoute>(IOutputStream& out, TTypeTraits<TDocRoute>::TFuncParam value) {
    out << value.ToDocId();
}

