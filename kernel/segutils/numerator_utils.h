#pragma once

#include <library/cpp/deprecated/dater_old/structs.h>

#include <kernel/segnumerator/utils/event_storer.h>
#include <kernel/segnumerator/segnumerator.h>
#include <library/cpp/html/pdoc/pds.h>
#include <library/cpp/mime/types/mime.h>
#include <kernel/hosts/owner/owner.h>

#include <kernel/recshell/recshell.h>

#include <util/generic/buffer.h>
#include <util/stream/buffer.h>
#include <util/string/cast.h>

namespace NSegutils {

class TUnknownEncoding: public yexception {
};

struct TDateTime {
    time_t Timestamp;
    NDater::TDaterDate Date;

    TDateTime() :
        Timestamp()
    {}

    void SetTimestamp(time_t t) {
        Timestamp = t;
        Date = NDater::TDaterDate::FromTimeT(t);
    }

    void SetDate(NDater::TDaterDate d) {
        Date = d;
        Timestamp = d.ToTimeT();
    }
};

struct THtmlDocument {
    TString Url;
    TString Html;
    TDateTime Time;

    ECharset HttpCharset;
    ECharset ForcedCharset;
    MimeTypes HttpMime;

    TString HttpLanguage;
    TString ForcedLanguage;

    mutable ECharset GuessedCharset;
    mutable TRecognizer::TLanguages GuessedLanguages;
    mutable TVector<TString> Errors;

    THtmlDocument()
    {
        Clear();
    }

    void Clear();
};

inline bool ValidCharset(ECharset c) {
    return c > CODES_UNKNOWN && c < CODES_MAX;
}

class TParserContext {
private:
    THtProcessor HtProcessor;
    THolder<TRecognizerShell> Recognizer;
    THolder<TOwnerCanonizer> OwnerCanonizer;
    NSegm::NPrivate::TSegContext SegContext;

public:
    explicit TParserContext(TStringBuf confdir)
        : HtProcessor()
    {
        TString dir = !confdir ? ToString(".") : ToString(confdir);
        HtProcessor.Configure((dir + "/htparser.ini").data());
        Recognizer.Reset(new TRecognizerShell(dir + "/dict.dict"));
        OwnerCanonizer.Reset(new TOwnerCanonizer(dir + "/2ld.list"));
    }

    TParserContext(TStringBuf htparserini, TStringBuf dictdict, TStringBuf _2ldlist)
        : HtProcessor()
    {
        HtProcessor.Configure(htparserini.data());
        if (!!dictdict)
            Recognizer.Reset(new TRecognizerShell(dictdict.data()));
        if (!!_2ldlist)
            OwnerCanonizer.Reset(new TOwnerCanonizer(ToString(_2ldlist)));
    }

    ~TParserContext() {
    }

    void SetUriFilterOff() {
        HtProcessor.Configure("option:URI_FILTER_TURN_OFF");
    }

    const TOwnerCanonizer* GetOwnerCanonizer() const {
        return OwnerCanonizer.Get();
    }

    template<typename TNumHandler>
    void SegNumerateDocument(TNumHandler* num, const THtmlDocument& html) {
        num->InitSegmentator(html.Url.data(), OwnerCanonizer.Get(), &SegContext);
        this->NumerateDocument(num, html);
    }

    template<typename TNumHandler>
    void NumerateDocument(TNumHandler* num, const THtmlDocument& html) {
        THolder<IParsedDocProperties> props(HtProcessor.ParseHtml(html.Html.data(), html.Html.size(), html.Url.data()));

        ECharset charset = CODES_UNKNOWN;
        if (ValidCharset(html.ForcedCharset) || !Recognizer.Get() && ValidCharset(html.HttpCharset)) {
            charset = ValidCharset(html.ForcedCharset) ? html.ForcedCharset : html.HttpCharset;
        } else {
            if (!Recognizer.Get())
                ythrow TUnknownEncoding () << "cannot recognize encoding, recognizer is uninitialized";

            TRecognizer::THints hints;
            hints.HttpCodepage = html.HttpCharset;
            hints.Url = html.Url.data();
            Recognizer->Recognize(HtProcessor.GetStorage().Begin(), HtProcessor.GetStorage().End(),
                html.GuessedCharset, html.GuessedLanguages, hints);
        }

        if (ValidCharset(charset)) {
            props->SetProperty(PP_CHARSET, NameByCharset(charset));
        } else if (ValidCharset(html.GuessedCharset)) {
            props->SetProperty(PP_CHARSET, NameByCharset(html.GuessedCharset));
        } else
            ythrow TUnknownEncoding () << "cannot recognize encoding";

        HtProcessor.NumerateHtml(*num, props.Get());
    }
};


template <typename TStorer>
class TSegmentatorCtx {
protected:
    typedef NSegm::TSegmentatorHandler<TStorer> TSegHandler;
private:
    TParserContext* Ctx;

    THolder<TSegHandler> Handler;
    THtmlDocument Doc;

public:
    explicit TSegmentatorCtx(TParserContext& ctx)
        : Ctx(&ctx)
        , Handler()
        , Doc()
    {
    }

    virtual ~TSegmentatorCtx()
    {}

    TSegHandler* GetSegHandler() const {
        return Handler.Get();
    }

    TSegHandler* SetDoc(const THtmlDocument& doc) {
        Clear();
        Doc = doc;
        Handler.Reset(new TSegHandler);
        return Handler.Get();
    }

    const THtmlDocument& GetDoc() const {
        return Doc;
    }

    THtmlDocument& GetDoc() {
        return Doc;
    }

    void NumerateDoc() {
        Ctx->SegNumerateDocument(Handler.Get(), Doc);
    }

    virtual void Clear() {
        Handler.Reset(nullptr);
    }
};

class TSegmentatorContext : public TSegmentatorCtx<NSegm::TEventStorer> {
public:
    explicit TSegmentatorContext(TParserContext& ctx)
        : TSegmentatorCtx<NSegm::TEventStorer>(ctx)
    {}

    const NSegm::TEventStorage& GetEvents(bool title) const {
        return GetSegHandler()->GetStorer().GetStorage(title);
    }

    NSegm::TEventStorage& GetEvents(bool title) {
        return GetSegHandler()->GetStorer().GetStorage(title);
    }

    const NSegm::TSegmentSpans& GetSegmentSpans() const {
        return GetSegHandler()->GetSegmentSpans();
    }

    const NSegm::TMainContentSpans& GetMainContentSpans() const {
        return GetSegHandler()->GetMainContentSpans();
    }

    const NSegm::THeaderSpans& GetHeaderSpans() const {
        return GetSegHandler()->GetHeaderSpans();
    }

    const NSegm::THeaderSpans& GetStrictHeaderSpans() const {
        return GetSegHandler()->GetStrictHeaderSpans();
    }

    const NSegm::TMainHeaderSpans& GetMainHeaderSpans() const {
        return GetSegHandler()->GetMainHeaderSpans();
    }

    const NSegm::TArticleSpans& GetArticleSpans() const {
        return GetSegHandler()->GetArticleSpans();
    }
};

}
