#include <kernel/qtree/request/fixreq.h>
#include <kernel/qtree/richrequest/richnode.h>

#include <library/cpp/getopt/opt.h>
#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/core/morpho_lang_discr.h>
#include <kernel/lemmer/dictlib/grambitset.h>
#include <kernel/lemmer/fixlist_load/from_text.h>

#include <library/cpp/charset/doccodes.h>
#include <library/cpp/langs/langs.h>
#include <library/cpp/charset/wide.h>
#include <util/generic/string.h>
#include <util/stream/file.h>
#include <util/stream/output.h>

TMorphoLangDiscriminator::TResult SimpleLangDisamb(const TRichRequestNode::TNodesVector& children, TLangMask preferableLanguage);
TMorphoLangDiscriminator::TResult MorphoLangDisamb(const TRichRequestNode::TNodesVector& children, const TDisambiguationOptions& options, const TLangMask& addMainLang);

struct TParams {
    enum TDiscriminator {
        DCurrent,
        DSimple,
        DMorpho
    };
    TString InFile;
    TString OutFile;
    TLangMask PreferedLangMask;
    TDiscriminator Discr;
    bool TryUtfInput;
    ECharset Encoding;
    TString FixList;
    TString LangFixList;
    bool PrintInitialTree;
    bool PrintQuality;
    bool PrintRequest;
    size_t IgnoreFilelds;

    TParams ()
        : Discr(DCurrent)
        , TryUtfInput(false)
        , Encoding(CODES_UTF8)
        , PrintInitialTree(false)
        , PrintQuality(false)
        , PrintRequest(false)
        , IgnoreFilelds(0)
    {
    }

};

static void usage(const char* progname) {
    TString shortname = progname;
    size_t pos = shortname.find_last_of("\\/:");
    if (pos != TString::npos)
        pos = shortname.find_first_not_of("\\/:", pos);
    if (pos != TString::npos)
        shortname = shortname.substr(pos);

    Cerr << "Usage: " << shortname << " [options] [<input file>]" << Endl;
    Cerr << "Recofnize language of requests" << Endl;
    Cerr << "Options:" << Endl;
    Cerr << "    -h, -?           Print this synopsis and exit" << Endl;
    Cerr << "    -e <encoding> Character encoding; default is Windows-1251" << Endl;
    Cerr << "    -u Try to convert input from utf8 first" << Endl;
    Cerr << "    -p <languages> List of prefered languages (comma-separated)" << Endl;
    Cerr << "    -s Simple discrimination" << Endl;
    Cerr << "    -m Morphology discrimination" << Endl;
    Cerr << "    -i Print initial tree (all languages)" << Endl;
    Cerr << "    -q Print quality" << Endl;
    Cerr << "    -f <fixlist> Morphology fixlist file" << Endl;
    Cerr << "    -l <langfixlist> Fixlist for morphology discrimination" << Endl;
    Cerr << "If no input file is specified, data is read from stdin." << Endl;
    Cerr << "If no output file is specified, results are sent to stdout." << Endl;
    Cerr << "The following languages are currently supported:" << Endl;

    const NLemmer::TLanguageVector& langlist = NLemmer::GetLanguageList();
    NLemmer::TLanguageVector::const_iterator it = langlist.begin();
    while (it != langlist.end()) {
        const TLanguage *lang = *(it++);
        if (!lang->Id)
            continue;
        Cerr << "    " << lang->Code() << ": " << lang->Name() << Endl;
    }
}

TLangMask ParseLanguageList(const TString& list) {
    TLangMask res;
    const char* language_separators = ",:; ";
    size_t pos = 0, endpos = 0;
    while (pos < list.length()) {
        pos = list.find_first_not_of(language_separators, endpos);
        if (pos == TString::npos)
            break;
        endpos = list.find_first_of(language_separators, pos);
        if (endpos == TString::npos)
            endpos = list.length();

        TString langname = list.substr(pos, endpos - pos);
        const TLanguage* lang = NLemmer::GetLanguageByName(langname.c_str());
        if (lang)
            res.Set(lang->Id);
        else
            Cerr << "Unrecognized language name " << langname << " - ignored";
    }
    return res;
}

static TUtf16String QualityString(NMorphoLangDiscriminator::TQuality q) {
    using namespace NMorphoLangDiscriminator;
    TUtf16String out;
    switch(q) {
        case QNo:
            out += '-';
            break;
        case QBad:
            out += 'F';
            break;
        case QAverage:
            out += 'B';
            break;
        case QPref:
            out += 'P';
            break;
        case QGood:
            out += 'G';
            break;
        case QAlmostGood:
            out += 'G';
            out += 'A';
            break;
        case QVeryGood:
            out += 'G';
            out += 'V';
            break;
        case QName:
            out += 'G';
            out += 'N';
            break;
        case QGoodFreq:
            out += 'G';
            out += 'C';
            break;
    }
    return out;
}


static TUtf16String PrintTree(const TRichRequestNode* root, const TParams& params,
    const NMorphoLangDiscriminator::TContext& discrList)
{
    using namespace NMorphoLangDiscriminator;
    TUtf16String out;
    if (root->WordInfo.Get()) {
        TLang2Qual qual;
        GetQuality(*(root->WordInfo), qual, discrList);
        out += root->WordInfo->GetNormalizedForm();
        out += '(';
        bool fl = false;
        for (TLang2Qual::const_iterator i = qual.begin(); i != qual.end(); ++i) {
            if (!root->WordInfo->GetLangMask().Test(i->first))
                continue;
            if (fl)
                out += ',';
            fl = true;
            out += ASCIIToWide(IsoNameByLanguage(i->first));
            if (params.PrintQuality) {
                out += ':';
                out += QualityString(i->second);
            }
        }
        out += ')';
    }
    if (root->Children.size()) {
        out += '{';
        for (size_t i = 0; i < root->Children.size(); ++i) {
            if (i)
                out += ' ';
            out += PrintTree(root->Children[i].Get(), params, discrList);
        }
        out += '}';
    }
    if (root->MiscOps.size()) {
        out += '[';
        for (size_t i = 0; i < root->MiscOps.size(); ++i) {
            if (i)
                out += ' ';
            out += PrintTree(root->MiscOps[i].Get(), params, discrList);
        }
        out += ']';
    }
    return out;
}


static TString RecodeInput(const TString& line, const TParams& params) {
    TChar* txt = (TChar*) alloca((line.length() + 1) * sizeof(TChar));
    size_t len = 0;
    if (params.Encoding == CODES_UTF8)
        return line;
    if (!params.TryUtfInput || !UTF8ToWide(line.c_str(), line.size(), txt, len)) {
        return WideToUTF8(CharToWide(line, params.Encoding));
    }
    return WideToUTF8(txt, len);
}

static void DisambiguatePhrase(TNodeSequence& children, const TDisambiguationOptions& options, TParams::TDiscriminator disc) {
    TLangMask langsToKill;
    if (disc ==  TParams::DSimple)
        langsToKill = SimpleLangDisamb(children, options.PreferredLangMask).Loosers;
    else if (disc ==  TParams::DMorpho) {
        langsToKill = MorphoLangDisamb(children, options, TLangMask()).Loosers;
    }
    else
        ythrow yexception() << "Unknown language";

    for (size_t chi = 0; chi < children.size(); ++chi) {
        TRichRequestNode* child = children[chi].Get();
        if (!IsWord(*child))
            continue;

        Y_ASSERT(child->WordInfo.Get());
        TWordNode* winfo = child->WordInfo.Get();

        if ((winfo->GetLangMask() & langsToKill).any() &&
             (winfo->GetLangMask() & ~langsToKill).any())
        {
            TWordInstanceUpdate(*winfo).FilterLemmas(langsToKill);
        }
    }
}

static void DisambiguatePhrases(TRichRequestNode* root, const TDisambiguationOptions& options, TParams::TDiscriminator disc) {
    for (size_t i = 0; i < root->Children.size(); ++i)
        DisambiguatePhrases(root->Children[i].Get(), options, disc);
    for (size_t i = 0; i < root->MiscOps.size(); ++i)
        DisambiguatePhrases(root->MiscOps[i].Get(), options, disc);
    switch (root->Op()) {
        case oAnd:
        case oOr:
        case oWeakOr:
            DisambiguatePhrase(root->Children, options, disc);
            break;
        default:
            break;
    }
}

static TString FixReq(const TString& src) {
    TString dest;
    if (!FixRequest(CODES_UTF8, src, &dest))
        dest = src;
    return dest;
}

void ProcessFile(IInputStream& in, IOutputStream& out, const TParams& params) {
    if (!!params.FixList)
        NLemmer::SetMorphFixList(params.FixList.c_str());
    NMorphoLangDiscriminator::TContext langFixList;
    if (!!params.LangFixList)
        langFixList.Init(params.LangFixList.c_str());

    TLanguageContext langContext(LI_DEFAULT_REQUEST_LANGUAGES | TLangMask(LANG_TUR, LANG_CZE), nullptr, TWordFilter::EmptyFilter);
    TCreateTreeOptions options(langContext);
    options.DisambOptions.SetDiscrList(langFixList);
    options.DisambOptions.PreferredLangMask = params.PreferedLangMask;

    TString line;
    while (in.ReadLine(line)) {
        try {
            TString u = RecodeInput(line, params);
            options.DisambOptions.FilterLanguages = false;
            TRichNodePtr treeInit;
            TUtf16String text;
            if (params.PrintRequest) {
                text += UTF8ToWide(u);
                text += '\n';
            }
            {
                size_t n = 0;
                size_t i = 0;
                while (n < params.IgnoreFilelds && i < u.length()) {
                    if (u[i] == '\t')
                        ++n;
                    ++i;
                }
                if (i)
                    u.remove(0, i);
            }
            const TString uf = FixReq(u);
            if (params.PrintInitialTree) {
                treeInit = CreateRichNode(UTF8ToWide(uf), options);
                text += PrintTree(treeInit.Get(), params, langFixList);
                text += '\n';
            }
            TRichNodePtr treeDiscr;
            switch (params.Discr) {
                case TParams::DCurrent:
                    options.DisambOptions.FilterLanguages = true;
                    treeDiscr = CreateRichNode(UTF8ToWide(uf), options);
                    break;
                case TParams::DSimple:
                case TParams::DMorpho:
                    treeDiscr = treeInit.Get() ? treeInit->Copy() :
                        CreateRichNode(UTF8ToWide(uf), options);
                    DisambiguatePhrases(treeDiscr.Get(), options.DisambOptions, params.Discr);
                    break;
            }
            text += PrintTree(treeDiscr.Get(), params, langFixList);
            text += '\n';
            out << WideToChar(text.data(), text.size(), params.Encoding) << Endl;
        } catch (const yexception& e) {
            Cout << "@error\n" << e.what() << Endl;
            continue;
        }
    }
}

int SelectOutput(IInputStream& in, const TParams& params) {
    if (params.OutFile.empty()) {
        ProcessFile(in, Cout, params);
        return 0;
    }

    THolder<IOutputStream> out;

    try {
        out.Reset(new TFixedBufferFileOutput(params.OutFile.c_str()));
    } catch (...) {
        Cerr << CurrentExceptionMessage() << ": " << params.OutFile << Endl;

        return 1;
    }

    ProcessFile(in, *out, params);

    return 0;
}

int SelectInput(const TParams& params) {
    if (params.InFile.empty())
        return SelectOutput(Cin, params);

    THolder<IInputStream> in;
    try {
        in.Reset(new TFileInput(params.InFile.c_str()));
    } catch (...) {
        Cerr << CurrentExceptionMessage() << ": " << params.InFile << Endl;
        return 1;
    }
    int result = SelectOutput(*in, params);
    return result;
}

int main(int argc, char* argv[])
{
    Opt opt(argc, argv, "hp:mse:uiqf:l:tk:");
    int optcode = EOF;

    TParams params;

    while ((optcode = opt.Get()) != EOF) {
        switch (optcode) {
        case '?':
        case 'h':
            usage(argv[0]);
            return 0;
        case 'p':
            params.PreferedLangMask = ParseLanguageList(opt.Arg);
            break;
        case 'm':
            params.Discr = TParams::DMorpho;
            break;
        case 's':
            params.Discr = TParams::DSimple;
            break;
        case 'e':
            params.Encoding = CharsetByName(opt.Arg);
            if (params.Encoding == CODES_UNKNOWN) {
                Cerr << "Unrecognized encoding: \"" << opt.Arg << "\"" << Endl;
                return 1;
            }
            break;
        case 'i':
            params.PrintInitialTree = true;
            break;
        case 'q':
            params.PrintQuality = true;
            break;
        case 't':
            params.PrintRequest = true;
            break;
        case 'u':
            params.TryUtfInput = true;
            break;
        case 'f':
            params.FixList = opt.Arg;
            break;
        case 'l':
            params.LangFixList = opt.Arg;
            break;
        case 'k':
            params.IgnoreFilelds = atoi(opt.Arg);
            break;
        }
    }

    if (opt.Ind < argc)
        params.InFile = argv[opt.Ind];

    return SelectInput(params);
}

