#include "dindex_tool.h"

#include <kernel/groupattrs/docsattrs.h>
#include <kernel/groupattrs/mutdocattrs.h>
#include <kernel/indexer/posindex/invcreator.h>
#include <kernel/tarc/docdescr/docdescr.h>
#include <kernel/tarc/iface/tarcio.h>
#include <kernel/walrus/advmerger.h>

#include <ysite/directtext/freqs/freqs.h>

#include <library/cpp/getopt/opt.h>
#include <kernel/keyinv/indexfile/indexstoragefactory.h>

#include <library/cpp/charset/recyr.hh>
#include <util/charset/wide.h>
#include <util/datetime/base.h>
#include <util/folder/dirut.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/string/type.h>
#include <util/string/vector.h>
#include <util/string/util.h>
#include <util/string/split.h>

const char* GPrefix[] = {"int_"};

using namespace NIndexerCore;

struct TDindexServer::TImpl {
    TIndexStorageFactory    Storage;
    TDTCreatorConfig        DtcCfg;
    TDirectTextCreator      Dtc;
    TInvCreatorConfig       InvCfg;
    TInvCreatorDTCallback   Ic;
    TFreqCalculator         FreqCalculator;
    TDirectIndexWithArchive DIndex;

    TImpl(size_t docCount, const TString& resDir, const TString& workDir, const TString& prefix)
        : Dtc(DtcCfg, TLangMask(LANG_RUS, LANG_ENG), LANG_RUS)
        , InvCfg(docCount)
        , Ic(&Storage, InvCfg)
        , FreqCalculator(false)
        , DIndex(Dtc, resDir + "arc")
    {
        Storage.InitIndexResources(resDir.data(), workDir.data(), prefix.data());
        DIndex.SetInvCreator(&Ic);
        DIndex.AddDirectTextCallback(&FreqCalculator);
    }
};

enum PrefixIndex {
    Integer = 0
};

TDindexServer::TDindexServer(const TString& name, const TString& prefix, size_t docCount, const TString& attrsNames)
    : Name(name)
    , DocId(0)
{
    WorkDir = "tmp-" + Name;
    MakeDirIfNotExist(WorkDir.c_str());

    ResultDir = "index-" + Name;
    MakeDirIfNotExist(ResultDir.c_str());

    ResultDir = ResultDir + GetDirectorySeparator() + "index";
    Prefix = prefix;
    DocCount = docCount;

    StringSplitter(attrsNames).Split(',').SkipEmpty().Collect(&AttrsNames);
}

TDindexServer::~TDindexServer() {
}

void TDindexServer::CreateIndex()
{
    Impl.Reset(new TImpl(DocCount, ResultDir, WorkDir, Prefix));
    AttrCreator.Reset(new NGroupingAttrs::TCreator(NGroupingAttrs::TConfig::Search));
    AttrCreator->Init(JoinStrings(AttrsNames, ",").c_str(), (ResultDir + NGroupingAttrs::TDocsAttrs::GetFilenameSuffix()).c_str());
}

void TDindexServer::Close()
{
    Impl->Ic.MakePortion();
    MergePortions(Impl->Storage);
    MakeArchiveDir(ResultDir + "arc", ResultDir + "dir");

    AttrCreator->Close();
    Y_UNUSED(AttrCreator.Release());
    Y_UNUSED(Impl.Release());
}

ECommand TDindexServer::GetCommand(const TString& command)
{
    typedef TMap<TString, ECommand> TCommandMap;
    static TCommandMap commandNames;
    static bool isInitialized = false;
    if (!isInitialized) {
        commandNames["InitServer"] = C_CREATE_SERVER;
        commandNames["CloseServer"] = C_CLOSE_SERVER;
        commandNames["DocStart"] = C_ADD_DOC;
        commandNames["DocFinish"] = C_COMMIT_DOC;
        commandNames["Attributes"] = C_ADD_ATTRIBUTES;
        commandNames["ZoneStart"] = C_OPEN_ZONE;
        commandNames["ZoneFinish"] = C_CLOSE_ZONE;
        commandNames["Text"] = C_ADD_TEXT;
        commandNames["IncBreak"] = C_INC_BREAK;
        commandNames["NextWord"] = C_NEXT_WORD;
        commandNames["Set"] = C_SET;
        isInitialized = true;
    }

    TCommandMap::const_iterator entry = commandNames.find(command);
    return (entry != commandNames.end()) ? entry->second : C_UNKNOWN;
}

void TDindexServer::EvalCommand(ECommand command, const TVector<TString>& tokens)
{
    assert(!!Impl);

    switch (command) {
    case C_ADD_DOC:
        if (tokens.size() < 2) {
            Impl->DIndex.AddDoc(DocId, LANG_UNK);
            ++DocId;
        } else {
            Impl->DIndex.AddDoc(DocId, LANG_UNK);
            ++DocId;
        }
        break;
    case C_COMMIT_DOC:
    {
        TFullArchiveDocHeader docHeader;
        TDocInfoEx docInfo;
        TFullDocAttrs docAttrs;
        docInfo.DocHeader = &docHeader;
        strcpy(docHeader.Url, "www.yandex.ru/test/");
        docHeader.Encoding = CODES_UTF8;
        docHeader.MimeType = MIME_HTML;

        // AttrCreator->Config() is not supposed to be changed here
        NGroupingAttrs::TMutableDocAttrs da(AttrCreator->Config(), DocId);
        for (TDirectIndex::TWAttributes::const_iterator attr = DocAttributes.begin(), end = DocAttributes.end();
            attr != end; ++attr)
        {
            docAttrs.AddAttr(attr->Name, WideToUTF8(attr->Value), TFullDocAttrs::AttrArcText);

            if (std::find(AttrsNames.begin(), AttrsNames.end(), attr->Name) != AttrsNames.end()) {
                da.SetAttr(attr->Name.c_str(), FromString<TCateg>(attr->Value));
            }
        }
        AttrCreator->AddDoc(da);

        DocAttributes.resize(0);

        char* endp;
        ui64 feedId(strtol(tokens.at(1).data(), &endp, 10));
        if (LastSystemError() == ERANGE) {
            ythrow yexception() << "Wrong value for prefix";
        }
        docInfo.FeedId = feedId;
        Impl->DIndex.CommitDoc(&docInfo, &docAttrs);
        break;
    }
    case C_OPEN_ZONE:
        // command, zoneName, attr0, value0, attr1, value1, ...
        if (tokens.size() < 2) {
            ythrow yexception() << "wrong number of command arguments";
        }
        Impl->DIndex.OpenZone(tokens.at(1), ParseAttributes(tokens, 2));
        break;
    case C_CLOSE_ZONE:
        if (tokens.size() < 2) {
            ythrow yexception() << "wrong number of command arguments";
        }
        Impl->DIndex.CloseZone(tokens.at(1));
        break;
    case C_ADD_TEXT:
        {
            TUtf16String seq = UTF8ToWide(tokens.at(1));
            Impl->DIndex.StoreText(seq.data(), seq.size(), BEST_RELEV);
        }
        break;
    case C_ADD_ATTRIBUTES:
        {   // command, attr0, value0, attr1, value1, ...
            TDirectIndex::TWAttributes attrs = ParseAttributes(tokens, 1);
            DocAttributes.insert(DocAttributes.end(), attrs.begin(), attrs.end());
            for (TDirectIndex::TWAttributes::const_iterator attr = attrs.begin(), end = attrs.end();
                attr != end; ++attr)
            {
                if (attr->Name.StartsWith(GPrefix[Integer]))
                    Impl->DIndex.StoreDocIntegerAttr(TDirectIndex::TWAttribute(attr->Name.substr(strlen(GPrefix[Integer])), attr->Value));
                else
                    Impl->DIndex.StoreDocAttr(*attr);
            }
            break;
        }
    case C_INC_BREAK:
        Impl->DIndex.IncBreak();
        break;
    case C_NEXT_WORD:
        Impl->DIndex.NextWord();
        break;
    case C_SET:
        if (tokens.size() < 3) {
            ythrow yexception() << "wrong number of command arguments";
        }
        {
            TString func = tokens.at(1);
            bool arg = !tokens.at(2) || IsTrue(tokens.at(2));// IsTrue won't return true on empty strings anymore
            if (func == "IgnoreStoreTextBreaks") {
                Impl->DIndex.SetIgnoreStoreTextBreaks(arg);
            } else if (func == "IgnoreStoreTextNextWord") {
                Impl->DIndex.SetIgnoreStoreTextNextWord(arg);
            }
        }
        break;
    default:
        ythrow yexception() << "unknown command";
        break;
    }
}

TDirectIndex::TWAttributes TDindexServer::ParseAttributes(const TVector<TString>& tokens, int firstPos) {
    if ((tokens.size() - firstPos) % 2 != 0) {
        ythrow yexception() << "attribute without a value";
    }
    const size_t attributesCount = (tokens.size() - firstPos) / 2;
    TDirectIndex::TWAttributes attributes(attributesCount);
    for (ui32 i = 0; i < attributesCount; ++i) {
        attributes.at(i).Name = tokens.at(firstPos + (2 * i));
        attributes.at(i).Value = UTF8ToWide(tokens.at(firstPos + (2 * i) + 1));
    }
    return attributes;
}

static TDindexServer* CreateServer(const TVector<TString>& tokens)
{
    TString name;
    TString prefix = "index-";
    TString attrsNames;
    int docCount = 50;

    switch (tokens.size()) {
    case 5:
        attrsNames = tokens.at(4);
    case 4:
        docCount = a2i(tokens.at(3));
    case 3:
        prefix = tokens.at(2);
    case 2:
        name = tokens.at(1);
        break;
    default:
        ythrow yexception() << "collection name is not specified";
    }

    THolder<TDindexServer> server(new TDindexServer(name, prefix, docCount, attrsNames));
    server->CreateIndex();
    return server.Release();
}

static void Run()
{
    // Simple (error-prone) read-eval loop.
    THolder<TDindexServer> server;
    IInputStream& src = Cin;
    TString line;
    while (src.ReadLine(line)) {
        TVector<TString> tokens;
        StringSplitter(line).Split('\t').Collect(&tokens);
        if (tokens.size() == 0) {
            continue;
        }

        const ECommand command = TDindexServer::GetCommand(tokens.at(0));
        switch (command) {
        case C_CREATE_SERVER:
            server.Reset(CreateServer(tokens));
            break;
        case C_CLOSE_SERVER:
            if (!!server) {
                server->Close();
            }
            return;
        default:
            if (!server) {
                ythrow yexception() << "call to not initialized server";
            }
            server->EvalCommand(command, tokens);
            break;
        }
    }
}

static void Usage(const char* progname)
{
    Cout << "Usage: " << progname << " [options]\n\
    Tool for creating index.\n\
    Reads input data through stdin.\n\
    Options:\n\
        -h    print this text\n\
    List of supported commands:\n\
        InitServer  - prepare for indexing;\n\
        CloseServer - end of indexing;\n\
        DocStart    - start of document;\n\
        DocFinish   - end of document;\n\
        Attributes  - add attributes to current document;\n\
        ZoneStart   - start a zone in a current documnert;\n\
        ZoneFinish  - end of zone;\n\
        Text        - add text;\n\
        IncBreak    - increase sentence break;\n\
        NextWord    - increase word break;\n\
        Set         - set indexer parameters.";
}

int main(int argc, char* argv[])
{
    Opt opt(argc, argv, "h");
    if (opt.Get() == 'h') {
        Usage(argv[0]);
    } else {
        Run();
    }
    return 0;
}
