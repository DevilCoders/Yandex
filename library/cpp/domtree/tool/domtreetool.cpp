#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/domtree/domtree.h>
#include <library/cpp/domtree/treedata.h>
#include <library/cpp/domtree/treetext.h>
#include <library/cpp/domtree/builder.h>
#include <library/cpp/domtree/numhandler.h>

#include <library/cpp/html/storage/storage.h>
#include <library/cpp/html/face/propface.h>
#include <library/cpp/html/html5/parse.h>
#include <library/cpp/token/nlptypes.h>
#include <library/cpp/token/token_structure.h>

#include <util/stream/file.h>
#include <util/folder/path.h>

namespace NDomTree {
    // hack: implementation lives in library/cpp/domtree/domtree.cpp
    TNumHandlerPtr TreeBuildingHandler(TTreeBuilderPtr builder);
}

static TString TokenMode(NDomTree::TTokenData::ETokenMode mode) {
    using namespace NDomTree;
    switch (mode) {
        case TTokenData::TM_SPACES:
            return "spaces";
        case TTokenData::TM_SPACES_EMPTY:
            return "empty";
        case TTokenData::TM_SINGLE:
            return "single";
        case TTokenData::TM_MULTI_START:
            return "mstart";
        case TTokenData::TM_MULTI_DELIM:
            return "mdelim";
        case TTokenData::TM_MULTI_BODY:
            return "mbody";
        case TTokenData::TM_UNTOK:
            return "untok";
    }
    return TString();
}

static NDomTree::TTreeBuilderPtr TreeData(IInputStream& in, const TString& url, const TString& charset) {
    TString html = in.ReadAll();
    NHtml::TStorage storage;
    {
        NHtml::TParserResult parsed(storage);
        NHtml5::ParseHtml(html, &parsed);
    }

    THolder<IParsedDocProperties> props(CreateParsedDocProperties());
    props->SetProperty(PP_BASE, url.data());
    props->SetProperty(PP_CHARSET, charset.data());

    NDomTree::TTreeBuilderPtr builder = NDomTree::CreateTreeBuilder();
    auto handlerPtr = NDomTree::TreeBuildingHandler(builder);
    Numerator numerator(*handlerPtr);
    numerator.Numerate(storage.Begin(), storage.End(), props.Get(), nullptr);

    return builder;
}

static TString PosOut(TPosting pos) {
    TStringStream out;
    out << GetBreak(pos) << "." << GetWord(pos);
    return out.Str();
}

void PrintTokens(const NDomTree::TTreeBuilderPtr& treeData, ui32 first, ui32 last, const TString& indent) {
    Cout << indent << "tokens (" << (last - first) << ")\n";
    for (size_t i = first; i < last; i++) {
        const NDomTree::TTokenData& td = treeData->TokenData(i);

        Cout << indent << "  token #" << i << "\n";
        Cout << indent << "    break type =";
        bool hasBreak = false;
        if (td.BreakType & ST_SENTBRK) {
            Cout << " SENT";
            hasBreak = true;
        }
        if (td.BreakType & ST_PARABRK) {
            Cout << " PARA";
            hasBreak = true;
        }
        if (!hasBreak) {
            Cout << " NONE";
        }
        Cout << "\n";
        Cout << indent << "    mode = " << TokenMode(td.TokenMode) << "\n";
        Cout << indent << "    pos = [" << PosOut(td.Pos) << "]\n";
        TWtringBuf txt;
        if (NDomTree::TStringPiece::SPS_TEXT == td.TokenText.Src) {
            txt = TWtringBuf(treeData->TextBuf().begin() + td.TokenText.Start, td.TokenText.Len);
        } else if (NDomTree::TStringPiece::SPS_NOINDEX == td.TokenText.Src) {
            txt = TWtringBuf(treeData->NoindexBuf().begin() + td.TokenText.Start, td.TokenText.Len);
        }
        Cout << indent << "    text = '" << txt << "'\n";
    }
}

static void PrintAttrs(const NDomTree::TTreeBuilderPtr& treeData, ui32 beg, ui32 cnt, const TString& indent) {
    using namespace NDomTree;
    Cout << indent << "attrs (" << cnt << ")\n";
    for (ui32 i = beg; i < (beg + cnt); i++) {
        const TAttrData& ad = treeData->AttrData(i);
        TStringBuf name(treeData->AttrBuf().begin() + ad.Name.Start, ad.Name.Len);
        TStringBuf value(treeData->AttrBuf().begin() + ad.Value.Start, ad.Value.Len);
        Cout << indent << "  attr #" << i
             << " '" << name << "' = '" << value << "'\n";
    }
}

static size_t GetDepth(const NDomTree::TTreeBuilderPtr& treeData, ui32 id, size_t depth = 0) {
    if (0 == id) {
        return depth;
    }
    return GetDepth(treeData, treeData->NodeData(id).Parent, depth + 1);
}

static void PrintTreeData(const NDomTree::TTreeBuilderPtr& treeData) {
    using namespace NDomTree;
    Cout << "tree stats:\n";
    Cout << "  node count      = " << treeData->NodeCount() << "\n";
    Cout << "  token count     = " << treeData->TokenCount() << "\n";
    Cout << "  attr count      = " << treeData->AttrCount() << "\n";
    Cout << "  text buf len    = " << treeData->TextBuf().size() << "\n";
    Cout << "  noindex buf len = " << treeData->NoindexBuf().size() << "\n";
    Cout << "  attr buf len    = " << treeData->AttrBuf().size() << "\n\n";

    for (ui32 i = 0; i < treeData->NodeCount(); i++) {
        const TNodeData& nd = treeData->NodeData(i);
        TString indent(static_cast<size_t>(GetDepth(treeData, i) * 2), ' ');
        Cout << indent << "node #" << i << "\n";
        Cout << indent << "    parent id = " << nd.Parent << "\n";
        if (EDomNodeType::NT_Text == nd.Type) {
            Cout << indent << "    type = text\n";
            Cout << indent << "    pos = ["
                 << PosOut(nd.Beg) << " : " << PosOut(nd.End)
                 << "]\n";
            PrintTokens(treeData, nd.FirstToken, nd.LastToken, indent + "    ");
        } else {
            Cout << indent << "    type = tag\n";
            const NHtml::TTag& tag = NHtml::FindTag(nd.HtmlTag);
            if (nullptr != tag.lowerName) {
                Cout << indent << "    name = " << tag.lowerName << "\n";
            } else {
                Cout << indent << "    name = <unknown>\n";
            }
            Cout << indent << "    siblings ids = [" << nd.PrevSibling << ", " << nd.NextSibling << "]\n";
            Cout << indent << "    child cnt = " << nd.ChildCnt << "\n";
            Cout << indent << "    children ids = [" << nd.ChildBeg << ", " << nd.ChildEnd << "]\n";
            PrintAttrs(treeData, nd.AttrBeg, nd.AttrCnt, indent + "    ");
        }
    }
}

static void PrintText(const NDomTree::TTreeBuilderPtr& treeData) {
    Cout << "Text:    '" << treeData->TextBuf() << "'\n";
    Cout << "Noindex: '" << treeData->NoindexBuf() << "'\n";
    Cout << "Attrs:   '" << treeData->AttrBuf() << "'\n";
}

static void PrintTree(const NDomTree::IDomNode* node, size_t indent = 0) {
    Y_ASSERT(nullptr != node);
    using namespace NDomTree;
    TString ind(static_cast<size_t>(indent * 2), ' ');
    switch (node->Type()) {
        case EDomNodeType::NT_Element: {
            const NHtml::TTag& tag = NHtml::FindTag(node->Tag());

            Cout << ind << "<" << tag.lowerName;
            for (const IDomAttr& a : node->Attrs()) {
                Cout << " " << a.Name() << "=\"" << a.Value() << "\"";
            }
            Cout << ">\n";
            for (const IDomNode& c : node->Children()) {
                PrintTree(&c, indent + 1);
            }
            Cout << ind << "</" << tag.lowerName << ">\n";
            break;
        }
        case EDomNodeType::NT_Text: {
            Cout << ind
                 << "<!--pos("
                 << PosOut(node->PosBeg()) << ":" << PosOut(node->PosEnd())
                 << ")-->"
                 << node->Text()
                 << "\n";
            break;
        }
    }
}

static void PrintNodeText(const NDomTree::INodeText* nt, const TString indent) {
    Y_ASSERT(nullptr != nt);
    using namespace NDomTree;
    Cout << indent << "node text:\n";
    Cout << indent << "  rawtext norm =  '" << nt->RawTextNormal() << "'\n";
    Cout << indent << "  rawtext all = \n";
    for (const auto& tchunk : nt->RawTextAll()) {
        Cout << indent << "    ";
        Cout << (tchunk.Noindex ? "N" : " ");
        Cout << " '" << tchunk.Text << "'\n";
    }
    Cout << indent << "  sents:\n";
    for (const auto& sent : nt->Sents()) {
        Cout << indent << "    " << sent.Text;
        if (sent.ParaBreak) {
            Cout << " <br/>";
        }
        Cout << "\n";
    }
    Cout << indent << "  tokens:\n";
    for (const auto& token : nt->Tokens(true)) {
        TStringStream ts;
        ts << "[";
        ts << (token.Noindex ? "N" : " ");
        ts << (token.Empty ? "e" : " ");
        switch (token.TokenType) {
            case TOKEN_WORD:
                ts << "w";
                break;
            case TOKEN_NUMBER:
                ts << "n";
                break;
            case TOKEN_MIXED:
                ts << "m";
                break;
            default:
                ts << " ";
                break;
        }
        ts << (token.MultiTokenStart ? "S" : (token.MultiTokenBody ? "B" : " "));
        ts << (token.ParaBreak ? "P" : (token.SentBreak ? "S" : " "));
        ts << "] [" << PosOut(token.Pos) << "] = '" << token.Text << "'";
        Cout << indent << "    " << ts.Str() << "\n";
    }
}

static void PrintTreeText(const NDomTree::IDomNode* node, size_t indent = 0) {
    Y_ASSERT(nullptr != node);
    using namespace NDomTree;
    TString ind(indent * 2, ' ');

    switch (node->Type()) {
        case EDomNodeType::NT_Element: {
            const NHtml::TTag& tag = NHtml::FindTag(node->Tag());

            Cout << ind << "<" << tag.lowerName << ">\n";
            PrintNodeText(node->NodeText(), ind + "  ");
            for (const IDomNode& c : node->Children()) {
                PrintTreeText(&c, indent + 1);
            }
            Cout << ind << "</" << tag.lowerName << ">\n";
            break;
        }
        case EDomNodeType::NT_Text: {
            Cout << ind
                 << "<!--pos("
                 << PosOut(node->PosBeg()) << ":" << PosOut(node->PosEnd())
                 << ")-->"
                 << node->Text()
                 << "\n";
            PrintNodeText(node->NodeText(), ind + "  ");
            break;
        }
    }
}

static void Process(const TString& infile, const TString& url, const TString& charset, const TString& mode) {
    Cout << "\n"
         << mode << "\t" << TFsPath(infile).GetName() << "\t" << charset << "\t" << url << "\n";
    Cout << "============================================================\n";
    TIFStream in(infile);
    if ("treedata" == mode) {
        PrintTreeData(TreeData(in, url, charset));
    } else if ("text" == mode) {
        PrintText(TreeData(in, url, charset));
    } else if ("tokens" == mode) {
        auto treeData = TreeData(in, url, charset);
        PrintTokens(treeData, 0, treeData->TokenCount(), TString());
    } else if ("tree" == mode) {
        auto tree = NDomTree::BuildTree(in, url, charset);
        using namespace NDomTree;
        const IDomNode* root = tree->GetRoot();
        PrintTree(root);
    } else if ("treetext" == mode) {
        auto tree = NDomTree::BuildTree(in, url, charset);
        using namespace NDomTree;
        const IDomNode* root = tree->GetRoot();
        PrintTreeText(root);
    } else {
        Cerr << "unknown mode " << mode << Endl;
    }
}

int main(int argc, char** argv) {
    using namespace NLastGetopt;

    TString url("http://127.0.0.1/");
    TString charset(NameByCharset(CODES_UTF8));
    TString infile;
    TString mode("tree");
    TString listfile;

    TOpts opts;
    opts.AddLongOption('u', "url", "document url, default = http://127.0.0.1/").StoreResult(&url);
    opts.AddLongOption('c', "charset", "default = utf-8").StoreResult(&charset);
    opts.AddLongOption('f', "infile", "input file with html").StoreResult(&infile);
    opts.AddLongOption('m', "mode", "text|tokens|tree|treetext|treedata, default = tree").StoreResult(&mode);
    opts.AddLongOption('l', "list", "list of input files").StoreResult(&listfile);
    opts.SetFreeArgsNum(0);
    TOptsParseResult(&opts, argc, argv);

    try {
        if (!listfile.Empty()) {
            TFileInput batch(listfile);
            TString line;
            while (batch.ReadLine(line)) {
                if (line.StartsWith('#')) {
                    continue;
                }
                TVector<TStringBuf> parts;
                Split(line, " \t", parts);
                if (parts.size() >= 3) {
                    infile = (TFsPath(listfile).Parent() / parts[0]).GetPath();
                    charset = parts[1];
                    url = parts[2];
                    Process(infile, url, charset, mode);
                }
            }
        } else if (!infile.Empty()) {
            Process(infile, url, charset, mode);
        } else {
            Cerr << "no list or input file\n";
        }
    } catch (yexception& e) {
        Cerr << "parser exception: " << e.what() << Endl;
    }
}
