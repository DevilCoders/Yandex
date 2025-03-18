/// @file Normalize html: create valid XML from HTML
///       usage: normalize_html (http://url | local/path)
///       Using props charset and converting to UTF8
///       Rewriting links against base
///       Removing xmlns: and non-wellformed attributes
///       Trim on </html>
/// @todo process visual markup <b> <i> etc...

#include "norm.h"

#include <library/cpp/html/entity/htmlentity.h>
#include <library/cpp/html/face/parsface.h>
#include <library/cpp/html/lexer/def.h>
#include <library/cpp/html/spec/tags.h>
#include <library/cpp/html/storage/storage.h>
#include <library/cpp/html/url/url.h>
#include <library/cpp/uri/http_url.h>
// #include <library/cpp/logger/all.h>

#include <util/generic/yexception.h>
#include <util/generic/string.h>
#include <util/generic/set.h>
#include <util/generic/map.h>
#include <util/memory/tempbuf.h>
#include <library/cpp/charset/ci_string.h>
#include <library/cpp/charset/recyr.hh>
#include <util/stream/output.h>
#include <util/system/maxlen.h>
#include <util/string/util.h>
#include <library/cpp/xml/encode/encodexml.h>

// #define NORM_DEBUG

namespace {
    using namespace NUri;
    using namespace NHtml;
    using TAttrs = TMap<TCiString, TUtf16String>;

    class TNormalizer {
    private:
        const TStorage* Storage;
        const IParsedDocProperties* Props;
        NUri::TUri Base;

        static const long UrlParseFlags = TUri::FeaturesRobot | TUri::FeatureConvertHostIDN;

    private:
        void NormalizeTag(const THtmlChunk& ev, IOutputStream& os);
        void NormalizeAttributes(HT_TAG tag, const TAttrs& attrs, IOutputStream& os);

    public:
        TNormalizer(const TStorage* storage, IParsedDocProperties* docProps)
            : Storage(storage)
            , Props(docProps)
        {
            const char* base = nullptr;
            int r = Props->GetProperty(PP_BASE, &base);
            if (r != 0)
                ythrow yexception() << "base url was not specified";
            Base.Parse(base, UrlParseFlags);
            if (!Base.IsValidAbs())
                ythrow yexception() << "base url is not valid or absolute";
        }
        void Write(IOutputStream& os);
    };

    static TUtf16String DoDecode(const char* text, size_t len, const IParsedDocProperties& parser) {
        TUtf16String Decoded;
        if (len) {
            Decoded.resize(len);
            wchar16* unibuf = Decoded.begin();
            unsigned lenbuf = HtEntDecodeToChar(parser.GetCharset(), text, len, unibuf);
            Decoded.resize(lenbuf);
        }
#ifdef NORM_DEBUG
        if (Decoded.find(0xFFFF) != TUtf16String::npos) {
            Cdbg << "found 0xFFFF in " << Decoded << Endl;
        }
#endif
        return Decoded;
    }

    static void NormalizeUtfText(TString wtext, IOutputStream& os) {
        os << EncodeXML(wtext.data(), true);
    }

    static bool IsXmlChar(wchar16 c) {
        // XML 1.0 specification, section 2.2:
        // Char ::= #x9 | #xA | #xD | [#x20-#xD7FF] | [#xE000-#xFFFD] | [#x10000-#x10FFFF]
        return c == 0x9 ||
               c == 0xA ||
               c == 0xD ||
               (c >= 0x0020 && c < 0xD800) ||
               (c >= 0xE000 && c < 0xFFFF);
    }

    static void NormalizeWideText(const wchar16* text, size_t len, IOutputStream& os) {
        TUtf16String buf;
        const wchar16* end = text + len;
        for (const wchar16* p = text; p < end; ++p)
            if (IsXmlChar(*p))
                buf += *p;
        TString wtext = WideToUTF8(buf);
        NormalizeUtfText(wtext, os);
    }

    static void NormalizeWideText(const TUtf16String& text, IOutputStream& os) {
        NormalizeWideText(text.data(), text.size(), os);
    }

    void GetAttrs(const THtmlChunk& e, TAttrs& attrs, const IParsedDocProperties& p) {
        for (size_t i = 0; i < e.AttrCount; ++i) {
            const NHtml::TAttribute& a = e.Attrs[i];
            TString name(e.text + a.Name.Start, a.Name.Leng);
            name.to_lower();
            TString nval(e.text + a.Value.Start, a.Value.Leng);
            attrs[name] = DoDecode(nval.data(), nval.size(), p);
        }
    }

    using TTagAttr = std::pair<HT_TAG, TCiString>;
    using TTagAttrs = TSet<TTagAttr>;
    TTagAttrs& UrlAttrs() {
        /// @todo: use singleton + array
        static TTagAttrs url_attrs;
        if (url_attrs.empty()) {
            url_attrs.insert(std::make_pair(HT_A, "href"));
            url_attrs.insert(std::make_pair(HT_LINK, "href"));

            url_attrs.insert(std::make_pair(HT_IMG, "src"));
            url_attrs.insert(std::make_pair(HT_FRAME, "src"));
            url_attrs.insert(std::make_pair(HT_IFRAME, "src"));
            url_attrs.insert(std::make_pair(HT_AREA, "href"));
            url_attrs.insert(std::make_pair(HT_FORM, "action"));
            url_attrs.insert(std::make_pair(HT_Q, "cite"));
            url_attrs.insert(std::make_pair(HT_BLOCKQUOTE, "cite"));
        }
        return url_attrs;
    }

    bool IsValidAttrName(const TString& name) {
        static str_spn ss_digit("0123456789");
        static str_spn ss_alnum("abcdefghijklmnopqrstuvwxyz"
                                "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
        static str_spn ss_else("._-");

        static str_spn ss_allowed(
            "0123456789"
            "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
            "._-");

        if (! name.size())
            return false;
        if (!isalpha(name[0]))
            return false;
        if (ss_allowed.cbrk(name.data()) != name.end())
            return false;
        if (name.StartsWith("xmlns"))
            return false;
        return true;
    }

    void TNormalizer::NormalizeAttributes(HT_TAG tag, const TAttrs& attrs, IOutputStream& os) {
        for (TAttrs::const_iterator i = attrs.begin(); i != attrs.end(); ++i) {
            if (!IsValidAttrName(i->first))
                continue;

            if (UrlAttrs().count(std::make_pair(tag, i->first))) {
                TString url = WideToUTF8(i->second);
#ifdef NORM_DEBUG
                Cdbg << "original url: " << url << Endl;
#endif
                TUri uri;
                /// @todo: set idna enc
                TUri::TLinkType ltype = NormalizeLinkForCrawl(Base, url.data(), &uri, CODES_UNKNOWN);

#ifdef NORM_DEBUG
                Cdbg << "url norm result: " << ltype << ", " << uri.PrintS() << Endl;
#endif

                if (ltype == TUri::LinkIsBad || !uri.IsValidAbs()) {
#ifdef NORM_DEBUG
                    Cerr << "bad url: " << uri.PrintS() << Endl;
#endif
                    continue;
                }

                os << ' ' << to_lower(i->first);
                os << "=\"";
                NormalizeUtfText(uri.PrintS(), os);
                os << "\"";
            } else {
                os << ' ' << to_lower(i->first);
                os << "=\"";
                NormalizeWideText(i->second, os);
                os << "\"";
            }
        }
    }

    void TNormalizer::NormalizeTag(const THtmlChunk& event, IOutputStream& os) {
        TAttrs attrs;
        GetAttrs(event, attrs, *Props);

        // Base
        if (event.Tag->is(HT_BASE) && attrs.count("href")) {
            TString url = WideToUTF8(attrs["href"]);
            TUri base;
            base.Parse(url);

#ifdef NORM_DEBUG
            Cdbg << "met base url     : " << base.PrintS() << Endl;
#endif

            if (base.IsValidAbs()) {
#ifdef NORM_DEBUG
                Cdbg << "setting base: " << base.PrintS() << Endl;
#endif
                Base = base;
            }
        }

        os << '<';
        if (event.flags.apos == HTLEX_END_TAG)
            os << '/';

        os << event.Tag->lowerName;

        if (event.flags.apos == HTLEX_START_TAG || event.flags.apos == HTLEX_EMPTY_TAG) {
            NormalizeAttributes(event.Tag->id(), attrs, os);
        } else if (event.flags.apos == HTLEX_END_TAG) {
        } else {
            ythrow yexception() << "wrong lextype (" << event.flags.apos << ") in NormalizeTag";
        }

        if (event.flags.apos == HTLEX_EMPTY_TAG) {
            os << " /";
        }

        os << '>';
    }

    void TNormalizer::Write(IOutputStream& os) {
        NHtml::TSegmentedQueueIterator first = Storage->Begin();
        NHtml::TSegmentedQueueIterator last = Storage->End();
        while (first != last) {
            const THtmlChunk* const ev = GetHtmlChunk(first);
            ++first;
            PARSED_TYPE t = PARSED_TYPE(ev->flags.type);
            if (t <= (int)PARSED_EOF)
                break;
            TUtf16String decoded = DoDecode(ev->text, ev->leng, *Props);
            if (t == PARSED_TEXT) {
                NormalizeWideText(decoded, os);
            } else if (t == PARSED_MARKUP && ev->flags.markup != MARKUP_IGNORED) {
                if (ev->flags.apos == HTLEX_TEXT)
                    os.Write(ev->text, ev->leng);
                else
                    NormalizeTag(*ev, os);
            } else if (t == PARSED_MARKUP && ev->flags.markup == MARKUP_IGNORED) {
                continue; // skip
            } else {
                ythrow yexception() << "unawaited type " << i32(t) << " in parse result event";
            }
            os.Flush();

            // trim trailing fragments
            if (t == PARSED_MARKUP && ev->flags.markup != MARKUP_IGNORED && ev->flags.apos == HTLEX_END_TAG && ev->Tag->id() == HT_HTML) {
                break;
            }
        }
    }

}

namespace NHtml {
    void ConvertChunksToXml(const TStorage* storage, IParsedDocProperties* docProps, IOutputStream& os) {
        TNormalizer norm(storage, docProps);
        norm.Write(os);
    }

}
