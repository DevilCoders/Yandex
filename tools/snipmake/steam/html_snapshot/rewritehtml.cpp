#include "rewritehtml.h"
#include <library/cpp/html/pcdata/pcdata.h>
#include <util/string/hex.h>

namespace NSteam
{

struct TTag;

struct TAttr
{
    const NHtml::TAttribute* SrcAttr;
    TTag& Tag;
    TString Name;
    TString Value;
    bool WasFixed;
    bool WasRemoved;

    TAttr(const NHtml::TAttribute* attr, TTag& tag);
    TAttr(const TString& name, TTag& tag);
    void ChangeValue(const TString& newValue);
    TString GetValue();
    void Remove();
};

struct TTag
{
    typedef TVector<TAttr> TAttrs;
    TAttrs Attrs;
    const THtmlChunk& Chunk;
    const TZoneEntry* Zone;
    HT_TAG Tag;
    bool Modified;

    TTag(const THtmlChunk& chunk, const TZoneEntry* zone)
        : Chunk(chunk)
        , Zone(zone)
        , Tag(chunk.Tag != nullptr ? chunk.Tag->id() : HT_any)
        , Modified(false)
    {
        Attrs.reserve(chunk.AttrCount);
        for (size_t i = 0; i < chunk.AttrCount; ++i) {
            const NHtml::TAttribute* attr = chunk.Attrs + i;
            Attrs.push_back(TAttr(attr, *this));
        }
    }

    TString GetUrl(const char* parserZoneAttrName)
    {
        TString result;
        if (!Zone) {
            return result;
        }
        for (size_t i = 0; i < Zone->Attrs.size(); ++i) {
            const TAttrEntry& attr = Zone->Attrs[i];
            if (!strcmp(~attr.Name, parserZoneAttrName)) {
                if (attr.Type == ATTR_URL) {
                    result = TString(~attr.Value);
                }
            }
        }
        return result;
    }

    TAttr* GetAttr(const char* name, bool createIfMissing = false)
    {
        for (TAttrs::iterator ii = Attrs.begin(), end = Attrs.end(); ii != end; ++ii) {
            if (ii->Name == name)
                return &(*ii);
        }
        if (createIfMissing) {
            Attrs.push_back(TAttr(name, *this));
            Modified = true;
            return &Attrs.back();
        }
        return nullptr;
    }

    bool WasModified()
    {
        return Modified;
    }

    TString GetFixedTag()
    {
        TString result;
        if (Attrs.empty()) {
            result.assign(Chunk.text, Chunk.leng);
            return result;
        }
        result = TString("<") + TStringBuf(Chunk.Tag->lowerName, Chunk.Tag->len());

        for (TAttrs::iterator ii = Attrs.begin(), end = Attrs.end(); ii != end; ++ii) {
            result += " ";
            if (ii->WasRemoved) {
                continue;
            }
            result += TString(ii->Name);
            if (ii->WasFixed) {
                result += "=\"" + ii->Value + "\"";
                continue;
            }
            const NHtml::TAttribute* srcAttr = ii->SrcAttr;
            Y_ASSERT(srcAttr != nullptr);
            if (!srcAttr->IsBoolean()) {
                result += "=";
                if (srcAttr->IsQuoted()) {
                    result.append(&srcAttr->Quot, 1);
                }
                result.append(ii->Value);
                if (srcAttr->IsQuoted()) {
                    result.append(&srcAttr->Quot, 1);
                }
            }
        }
        result.append(">");
        return result;
    }
};


TAttr::TAttr(const NHtml::TAttribute* attr, TTag& tag)
    : SrcAttr(attr)
    , Tag(tag)
    , Name(tag.Chunk.text + attr->Name.Start, attr->Name.Leng)
    , Value(tag.Chunk.text + attr->Value.Start, attr->Value.Leng)
    , WasFixed(false)
    , WasRemoved(false)
{
    Name.to_lower();
}

TAttr::TAttr(const TString& name, TTag& tag)
    : SrcAttr(nullptr)
    , Tag(tag)
    , Name(name)
    , WasFixed(true)
    , WasRemoved(false)
{
    Name.to_lower();
}

void TAttr::ChangeValue(const TString& newValue)
{
    Value = newValue;
    WasFixed = true;
    Tag.Modified = true;
}

void TAttr::Remove()
{
    WasRemoved = true;
    Tag.Modified = true;
}

TString TAttr::GetValue()
{
    TString result;
    if (!WasRemoved) {
        result = Value;
    }
    return result;
}

class THtmlRewriteHandler: public INumeratorHandler
{
private:
    ICssConverter& CssConverter;
    const TBuffer& Html;

    TBuffer Result;
    bool Working;

    bool InsideLiteral;
    HT_TAG CurrentLiteral;
    TString LiteralContent;
    THttpURL BaseUrl;
    ECharset Charset;
    IUrlMapper& Mapper;

public:
    THtmlRewriteHandler(IUrlMapper& urlMapper, ICssConverter& cssConverter, const TBuffer& html, const TString& currentUrl, ECharset charset)
        : CssConverter(cssConverter)
        , Html(html)
        , Working(true)
        , InsideLiteral(false)
        , CurrentLiteral(HT_any)
        , Charset(charset)
        , Mapper(urlMapper)
    {

        Result.Reserve(1000000);
        bool urlValid = MergeUrlWithBase(currentUrl, BaseUrl, BaseUrl, Charset);
        Y_ASSERT(urlValid);
    }

    TBuffer& GetResult()
    {
        return Result;
    }

    void FixJSAttrs(TTag& /*tag*/)
    {
        //TODO
    }

    void FixStyle(TTag& tag)
    {
        TAttr* style = tag.GetAttr("style");
        if (!style) {
            return;
        }
        TString styleText = DecodeHtmlPcdata(style->GetValue());
        styleText = CssConverter.ConvertCss(styleText, BaseUrl, Charset, true);
        styleText = EncodeHtmlPcdata(styleText);
        style->ChangeValue(styleText);
    }

    void FixImg(TTag& tag, const char* hrefAttrName)
    {
        TString imageUrl = tag.GetUrl("image");
        TAttr* attr = tag.GetAttr(hrefAttrName);
        if (!imageUrl || !attr) {
            return;
        }
        attr->ChangeValue(Mapper.RewriteUrl(imageUrl, IUrlMapper::LINK_IMAGE));
    }

    void FixLink(TTag& tag)
    {
        TString cssUrl = tag.GetUrl("style");
        TAttr* attr = tag.GetAttr("href");

        if (!cssUrl || !attr) {
            return;
        }

        TAttr* rel = tag.GetAttr("rel");
        IUrlMapper::ELinkType linkType = IUrlMapper::LINK_UNKNOWN;
        if (rel && rel->GetValue() == "stylesheet") {
            linkType = IUrlMapper::LINK_CSS;
        }

        attr->ChangeValue(Mapper.RewriteUrl(cssUrl, linkType));
    }

    void FixScript(TTag& tag)
    {
        TAttr* attr = tag.GetAttr("src");
        if (attr) {
            attr->Remove();
        }
    }

    void FixFrame(TTag& tag)
    {
        TString frameUrl = tag.GetUrl("link");
        if (!frameUrl) {
            frameUrl = tag.GetUrl("linkint");
        }
        TAttr* attr = tag.GetAttr("src");
        if (!frameUrl || !attr) {
            return;
        }
        attr->ChangeValue(Mapper.RewriteUrl(frameUrl, IUrlMapper::LINK_FRAME));
    }

    void AddGuid(TTag& tag)
    {
        if (tag.Tag == HT_HTML) {
            return;
        }
        if (tag.Chunk.text < Html.Data() || tag.Chunk.text > Html.Data() + Html.Size()) {
            return;
        }
        ptrdiff_t offset = tag.Chunk.text - Html.Data();

        tag.GetAttr("data-guid", true)->ChangeValue(ToString(offset));
    }

    void FixTag(TTag& tag)
    {
        AddGuid(tag);
        FixJSAttrs(tag);
        switch (tag.Tag) {
        case HT_IMG:
        case HT_INPUT:
            FixImg(tag, "src");
            FixStyle(tag);
            break;
        case HT_META:
            FixImg(tag, "content");
            break;
        case HT_LINK:
            FixLink(tag);
            break;
        case HT_FRAME:
        case HT_IFRAME:
            FixFrame(tag);
            FixStyle(tag);
            break;
        case HT_SCRIPT:
            FixScript(tag);
            break;
        default:
            FixImg(tag, "background");
            FixStyle(tag);
            break;
        }
    }

    TString FixLiteral(const TString& content, HT_TAG type)
    {
        if (type == HT_STYLE) {
            return CssConverter.ConvertCss(content, BaseUrl, Charset, false);
        }
        else if (type == HT_SCRIPT) {
            return "";
        }
        else {
            return content;
        }
    }

    void OnMoveInput(const THtmlChunk& chunk, const TZoneEntry* zone, const TNumerStat&) override
    {
        if (!Working || chunk.flags.type == PARSED_EOF) {
            Working = false;
            return;
        }

        HT_TAG htTag = chunk.Tag ? chunk.Tag->id() : HT_any;

        bool need_out = false;
        bool doStartLiteral = false;

        if (chunk.flags.type == PARSED_TEXT) {
            need_out = true;
        }
        else if (chunk.flags.type == PARSED_MARKUP && chunk.GetLexType() == HTLEX_END_TAG && htTag == CurrentLiteral) {
            need_out = true;
            TString fixedLiteral = FixLiteral(LiteralContent, CurrentLiteral);
            CurrentLiteral = HT_any;
            InsideLiteral = false;
            LiteralContent = TString();

            Result.Append(fixedLiteral.data(), fixedLiteral.size());
        }
        else if (chunk.flags.type == PARSED_MARKUP && chunk.flags.markup != MARKUP_IMPLIED) {
            if ((chunk.GetLexType() != HTLEX_START_TAG && chunk.GetLexType() != HTLEX_EMPTY_TAG)
                || !chunk.Tag
                || chunk.Tag->id() == HT_any)
            {
                need_out = true;
            }
            else if (htTag == HT_BASE) {
                // remove BASE, but adjust the base URL
                TTag tag(chunk, zone);
                TString baseUrl = tag.GetUrl("base");
                MergeUrlWithBase(baseUrl, BaseUrl, BaseUrl, Charset);
            }
            else {
                TTag tag(chunk, zone);
                FixTag(tag);
                if (tag.WasModified()) {
                     need_out = false;
                     TString fixedTag = tag.GetFixedTag();
                     if (InsideLiteral) {
                         LiteralContent += fixedTag;
                     }
                     else {
                         Result.Append(fixedTag.data(), fixedTag.size());
                     }
                }
                else {
                    need_out = true;
                }

                if (!InsideLiteral && (htTag == HT_STYLE || htTag == HT_SCRIPT)) {
                    CurrentLiteral = htTag;
                    doStartLiteral = true;
                }
            }
        }

        if (need_out) {
            TStringBuf chunkStr(chunk.text, chunk.leng);
            if (InsideLiteral) {
                LiteralContent += chunkStr;
            }
            else {
                Result.Append(chunkStr.data(), chunkStr.size());
            }
        }

        InsideLiteral |= doStartLiteral;
    }
};


TConvertResult THtmlRewriter::TryConvert(const TPreparsedHtml& html, IUrlMapper& mapper, ICssConverter& cssConverter)
{
    TConvertResult result;
    if (!html.IsValid()) {
        return result;
    }

    TAttributeExtractor extractor(&HtConfig);
    THtmlRewriteHandler convHandler(mapper, cssConverter, html.GetHtml(), html.GetUrl(), html.GetCharset());
    Numerator numerator(convHandler);

    bool numeratorOK = html.Numerate(numerator, extractor);

    if (!numeratorOK) {
        result.ErrorMessage = "HTML parser failed";
    }
    else {
        result.Done = true;
        convHandler.GetResult().AsString(result.Data);
    }

    return result;
}
}
