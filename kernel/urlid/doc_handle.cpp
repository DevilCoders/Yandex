#include "doc_handle.h"
#include "urlid.h"

#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/ysaveload.h>
#include <util/string/cast.h>

namespace {
    constexpr char IndexDelimiter = ',';
}

const TDocHandle::THash TDocHandle::InvalidHash = static_cast<TDocHandle::THash>(-1);

template <>
void In(IInputStream& src, TDocHandle& target) {
    TString s;
    src >> s;
    target.FromString(s);
}

void TDocHandle::Load(IInputStream* str) {
    ::Load(str, DocRoute.Raw());
    bool isString;
    ::Load(str, isString);
    if (isString) {
        DocHash = TString();
        ::Load(str, std::get<TString>(DocHash));
    } else {
        DocHash = InvalidHash;
        ::Load(str, std::get<THash>(DocHash));
    }
    ::Load(str, IndexGeneration);
}


void TDocHandle::Save(IOutputStream* str) const {
    ::Save(str, DocRoute.Raw());
    ::Save(str, IsString());
    if (IsString()) {
        ::Save(str, std::get<TString>(DocHash));
    } else {
        ::Save(str, std::get<THash>(DocHash));
    }
    ::Save(str, IndexGeneration);
}

/** @brief Validate that @c dh.ToString() means the same as @c originalStr
 *
 * This function requires that ToString() is the same as originalStr,
 * but if there were leading zeros in DocRoute, they may be omitted.
 */
static void ValidateDeserializarion(const TDocHandle& dh, TStringBuf originalStr)
{
#ifdef NDEBUG
    Y_UNUSED(dh);
    Y_UNUSED(originalStr);
#else
    const TString& parsedStr = dh.ToString(TDocHandle::PrintAll);
    const TStringBuf fullOrigStr(originalStr);

    if (dh.IsString()) {
        Y_VERIFY_DEBUG(
            parsedStr == originalStr,
            "TDocHandle('%s').ToString() = '%s', differs from original, while parsed as verbatim string",
            TString(fullOrigStr).c_str(), parsedStr.c_str()
        );
        return;
    }

    TStringBuf parsed(parsedStr);
    TStringBuf origHash = originalStr.RNextTok(TDocRoute::Separator), origRouter;
    TStringBuf parsHash = parsed.RNextTok(TDocRoute::Separator), parsRouter;
    Y_VERIFY_DEBUG(
        origHash == parsHash,
        "hashes differ in '%s' (original) vs '%s' (parsed)\n  origHash: '%s', parsHash: '%s'",
        TString(fullOrigStr).c_str(), parsedStr.c_str(),
        TString(origHash).c_str(), TString(parsHash).c_str()
    ); //< IndGen included, actually

    do {
        bool haveOrigToken = originalStr.RNextTok(TDocRoute::Separator, origRouter);
        bool haveParsToken = parsed.RNextTok(TDocRoute::Separator, parsRouter);

        Y_VERIFY_DEBUG(
            haveOrigToken == haveParsToken,
            "route is of different length in '%s' (original) vs '%s' (parsed):\n  haveOrigToken = %d, haveParsToken = %d",
            TString(fullOrigStr).c_str(), parsedStr.c_str(),
            haveOrigToken, haveParsToken
        );

        if (!haveOrigToken)
            return;

        while(origRouter.size() > 1 && origRouter.StartsWith('0'))  // do not strip '0' into ''
            origRouter.Skip(1);

        Y_VERIFY_DEBUG(
            origRouter == parsRouter,
            "route differs in '%s' (original) vs '%s' (parsed):\n  origRouter = '%s', parsRouter = '%s'",
            TString(fullOrigStr).c_str(), parsedStr.c_str(),
            TString(origRouter).c_str(), TString(parsRouter).c_str()
        );
    } while (true);
#endif
}

void TDocHandle::FromString(TStringBuf str) {
    if (Y_UNLIKELY(str.EndsWith(IndexDelimiter)))
        ythrow yexception() << "Incorrect index generation value: docid " << TString(str).Quote() << " ends with " << IndexDelimiter;
    const TStringBuf fullStr = str;
    TStringBuf routeAndHash = str.NextTok(IndexDelimiter);
    TStringBuf strIndex = str; // consume everything after the first comma
    TStringBuf hashStr;

    DocRoute = TDocRoute::FromDocId(routeAndHash, &hashStr);

    if (Y_UNLIKELY(!hashStr)) {
        DocHash = InvalidHash;
        return;
    }

    if (hashStr[0] == 'Z' && hashStr.size() % 2 == 1) {
        hashStr.Skip(1);
        SetUnique(true);
    } else {
        SetUnique(false);
    }
    // Some docids are actualy strings of some random format.
    // In future this code should be deleted, and docids should become numeric hashes.
    if (IsUnique()) {
        if (Y_LIKELY(hashStr.size() == 16)) {
            try {
                DocHash = NUrlId::Str2Hash(hashStr);
            } catch (...) {
                DocHash = ::ToString(hashStr);
            }
        } else {
            DocHash = ::ToString(hashStr); //< prevent leading zeros to appear on serializing
        }
    } else {
        THash hash;
        bool convertibleFromString = ::TryFromString(hashStr, hash);
        if (Y_UNLIKELY(!convertibleFromString || (hashStr.size() > 1 && hashStr[0] == '0' || hashStr[0] == '+'))) {
            DocHash = ::ToString(hashStr); //< carefully preserve leading zeros, save it as a string.
        } else {
            DocHash = hash;
        }
    }

    if (!strIndex) {
        IndexGeneration = UndefIndGenValue;
    } else if(Y_UNLIKELY(strIndex[0] == '0' || strIndex[0] == '+' || !::TryFromString(strIndex, IndexGeneration))) {
        DocHash = TString(hashStr) + IndexDelimiter + strIndex;
        IndexGeneration = UndefIndGenValue;
    }

    // Test that this dochandle representation supports any input string correctly if no exceptions thrown
    ValidateDeserializarion(*this, fullStr);
}

template <>
TDocHandle FromStringImpl<TDocHandle, char>(const char* str, size_t sz) {
    TDocHandle dh(TStringBuf(str, sz));
    return dh;
}

TDocHandle::THash TDocHandle::IntHash() const {
    if (!IsString())
        return std::get<THash>(DocHash);
    const TString& strHash = std::get<TString>(DocHash);
    if (IsUnique())
        return NUrlId::Str2Hash(strHash);
    else
        return ::FromString<THash>(std::get<TString>(DocHash));
}

bool TDocHandle::TryIntHash(THash& result) const {
    if (!IsString()) {
        result = std::get<THash>(DocHash);
        return true;
    }
    const TString& strHash = std::get<TString>(DocHash);
    if (IsUnique()) {
        bool correctStr = strHash.size() % 2 == 0;
        if (correctStr) {
            try {
                result = NUrlId::Str2Hash(strHash);
            } catch (...) {
                correctStr = false;
            }
        }
        return correctStr;
    }
    return ::TryFromString<THash>(std::get<TString>(DocHash), result);
}

namespace {
template <class F>
void Serialize(TTypeTraits<TDocHandle>::TFuncParam handle, F&& print, TDocHandle::EFormat format) {
    const auto sep = TDocRoute::Separator; //< avoid linker error (IOutputStream fails on 'static constexpr')
    if ((format & TDocHandle::PrintZDocId) && !handle.IsUnique()) {
        format = static_cast<TDocHandle::EFormat>(format | TDocHandle::PrintRoute);
    }

    if (handle.DocRoute && (format & TDocHandle::PrintRoute)) {
        auto printRoute = [&print, &sep](const TDocRoute& r) {
            print(r.ToDocId());
            if (r)
                print(sep);
        };
        if ((format & TDocHandle::PrintClientDocId) == TDocHandle::PrintClientDocId) {
            TDocRoute r = handle.DocRoute;
            r.PopFront();
            printRoute(r);
        } else {
            printRoute(handle.DocRoute);
        }
    }

    if (handle.DocHash == std::variant<ui64, TString>(TDocHandle::InvalidHash)) {
        // Print nothing if doc hash is not defined
    } else {
        if (handle.IsUnique() && (format & TDocHandle::OmitZ) == 0) {
            print("Z");
        }

        if (Y_UNLIKELY(handle.IsString())) {
            print(std::get<TString>(handle.DocHash));
        } else {
            TDocHandle::THash h = std::get<TDocHandle::THash>(handle.DocHash);
            if (handle.IsUnique()) {
                print(NUrlId::Hash2Str(h));
            } else {
                print(h);
            }
        }
    }

    if ((format & TDocHandle::AppendIndGeneration) && handle.IndexGeneration != UndefIndGenValue && handle.IndexGeneration != ErrorIndGenValue) {
        print(IndexDelimiter);
        print(handle.IndexGeneration);
    }
}
}

template <>
void Out<TDocHandle>(IOutputStream& out, TTypeTraits<TDocHandle>::TFuncParam value) {
    Serialize(value, [&out](auto&& text){out << text;}, TDocHandle::PrintAll);
}

namespace {
template <class T> void append(TString& to, T&& what){ to += what; }
void append(TString& to, ui32 what){ char buf[32]; to.append(TStringBuf(buf, ToString(what, buf, 32))); }
void append(TString& to, TDocHandle::THash what){ char buf[64]; to.append(TStringBuf(buf, ToString(what, buf, 64))); }
}

TString TDocHandle::ToString(EFormat format) const {
    // avoid reallocations for all Z-docids (this is the production mainstream)
    TString ans(Reserve(sizeof('Z') + 16 + sizeof('\0')));
    Serialize(*this, [&ans](auto&& text){append(ans, text);}, format);
    return ans;
}
