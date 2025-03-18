#include <ysite/yandex/filter/urlhilite.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/html/pdoc/pds.h>
#include <library/cpp/numerator/numerate.h>

#include <util/stream/file.h>
#include <util/stream/output.h>

class TVerboseNumeratorHandler
    : public INumeratorHandler
{
private:
    static TString GetText(const THtmlChunk& e) {
        return TString(e.text, e.leng);
    }

public:
    void OnTextStart(const IParsedDocProperties*) override {
        Cdbg << "TextStart" << Endl;
    }
    void OnTextEnd(const IParsedDocProperties*, const TNumerStat&) override {
        Cdbg << "TextEnd" << Endl;
    }
    void OnParagraphEnd() override {
        Cdbg << "ParagraphEnd" << Endl;
    }
    void OnAddEvent(const THtmlChunk& e) override {
        Cdbg << "AddEvent" << ' '
             << PARSED_TYPE(e.flags.type) << ' '
             << GetText(e)
             << Endl;
    }
    void OnRemoveEvent(const THtmlChunk& e) override {
        Cdbg << "RemoveEvent" << ' '
             << GetText(e)
             << Endl;
    }

    void OnTokenStart(const TWideToken& t, const TNumerStat&) override {
        Cdbg << "TokenStart" << ' '
             << TUtf16String(t.Token, t.Leng)
             << Endl;
    }
    void OnSpaces(TBreakType, const TChar*, unsigned, const TNumerStat&) override {
        Cdbg << "Spaces" << Endl;
    }
    void OnMoveInput(const THtmlChunk&, const TZoneEntry* zone, const TNumerStat&) override {
        if (zone) {
            if (zone->Name && !zone->OnlyAttrs) {
                if (zone->IsOpen)
                    Cdbg << "Zone open "
                         << zone->Name
                         << Endl;
                if (zone->IsClose) // Zone may be opened and closed in one entry
                    Cdbg << "Zone close "
                         << zone->Name
                         << Endl;
            }
            for (size_t i = 0; i < zone->Attrs.size(); ++i) {
                Cdbg << "Attr" << ' '
                     << ~zone->Attrs[i].Name << " = "
                     << zone->Attrs[i].DecodedValue.data()
                     << Endl;
            }
        }
    }
};

void DumpNumerateFile(const TString& inputFile, const TString& config) {
    THtProcessor htProcessor;
    if (config == "null")
        htProcessor.Configure(nullptr);
    else if (config.size())
        htProcessor.Configure(config.data());

    TIFStream is(inputFile);
    THolder<IParsedDocProperties> docProps(htProcessor.ParseHtml((IInputStream*)&is, "http://127.0.0.1/"));
    docProps->SetProperty(PP_CHARSET, "utf-8");

    TVerboseNumeratorHandler numerator;
    htProcessor.NumerateHtml(numerator, docProps.Get());
}

class TSimpleHiliter
    : public IHiliter
{
private:
    TString StartMark, EndMark;
public:
    TSimpleHiliter(const TString& start = "[HL]", const TString& end = "[/HL]")
        : StartMark(start)
        , EndMark(end)
    {
    }
    void Insert(const HILITE *hilite) override {
        const HILITE_TYPE type = hilite->type;
        if (HILITE_FIRST_WORD_START <= type && type <= HILITE_LAST_WORD_END) {
            HTML_HILITER_COOKIE *cookie = (HTML_HILITER_COOKIE*)hilite->hinfo;
            if (cookie && cookie->IsCDATA)
                return;
            if(cookie && cookie->Anchor)
                Cout << "</A>";

            switch (type) {
                case HILITE_FIRST_WORD_START:
                case HILITE_WORD_START:
                    Cout << StartMark;
                    break;
                case HILITE_WORD_END:
                case HILITE_LAST_WORD_END:
                    Cout << EndMark;
                    break;
                default:
                    break;
            }
            if(cookie && cookie->Anchor)
                Cout << cookie->Anchor;
        }
    }
    void Write(const void* buf, size_t size) override {
        Cout.Write(buf, size);
    }
};

template<>
void Out<HILITE_TYPE>(IOutputStream& os, TTypeTraits<HILITE_TYPE>::TFuncParam n) {
    static const char* HILITE_TYPE_names[8] = { "HILITE_FIRST_WORD_START", "HILITE_WORD_START", "HILITE_WORD_END", "HILITE_LAST_WORD_END", "HILITE_TEXT_START",
"HILITE_TEXT_END", "HILITE_INITIALIZE", "HILITE_SPECIAL", };

    if( 0 > n || n >= 8 )
        os << "(out of range)";
    else
        os << HILITE_TYPE_names[n];
}


class TVerboseHiliter
    : public IHiliter
{
public:
    void Insert(const HILITE *hilite) override {
        Cout << '[' << hilite->type << ", off: " << hilite->offset;
        if(hilite->hinfo) {
            HTML_HILITER_COOKIE& cookie = *(HTML_HILITER_COOKIE*)hilite->hinfo;
            if(cookie.IsCDATA)
                Cout << ", LIT";
            if(cookie.Anchor)
                Cout << ", link: " << cookie.Anchor;
        }
        Cout << ']';
    }
    void Write(const void* buf, size_t size) override {
        Cout.Write(buf, size);
    }
};

void HiliteFile(const TString& file, const TString& query,
                const TString& fileCharset, const TString& /*config*/, bool verbose,
                const TString& startMark, const TString& endMark)
{
    TIFStream is(file);
    TDocMetaData md;
    md.Coding = CharsetByName(fileCharset.data());

    THolder<IHiliter> hiliter;
    if(verbose)
        hiliter.Reset(new TVerboseHiliter);
    else
        hiliter.Reset(new TSimpleHiliter(startMark, endMark));

    ui32 softness = 0;
    UrlHilite(hiliter.Get(), &is, query.data(), 0, TBinaryRichTree(), TString(""), md, &softness);
}

int main(int argc, char** argv) {

    TString config;
    TString startMark = "[HL]";
    TString endMark = "[/HL]";
    TString charset = "UTF-8";
    bool dump = false;
    bool verbose = false;

    using namespace NLastGetopt;
    TOpts opts;

    opts.AddHelpOption();
    opts.SetFreeArgsNum(2);
    opts.SetFreeArgTitle(0, "input.html");
    opts.SetFreeArgTitle(1, "'request text'");

    opts.AddCharOption('c', REQUIRED_ARGUMENT, "parser config    [builtin]").StoreResult(&config);
    opts.AddCharOption('d', NO_ARGUMENT,       "dump");
    opts.AddCharOption('v', NO_ARGUMENT,       "verbose");
    opts.AddCharOption('e', REQUIRED_ARGUMENT, "encoding         [utf-8]").StoreResult(&charset);
    opts.AddCharOption('S', REQUIRED_ARGUMENT, "start mark       [HL]").StoreResult(&startMark);
    opts.AddCharOption('E', REQUIRED_ARGUMENT, "end mark         [/HL]").StoreResult(&endMark);

    TOptsParseResult opr(&opts, argc, argv);

    dump = opr.Has('d');
    verbose = opr.Has('v');

    if(dump)
        DumpNumerateFile(opr.GetFreeArgs().at(0), config);

    const TString filename = opr.GetFreeArgs().at(0);
    const TString reqtext  = opr.GetFreeArgs().at(1);
    HiliteFile(filename, reqtext, charset, config, verbose, startMark, endMark);
}

