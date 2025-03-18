#include <kernel/search_daemon_iface/reqtypes.h>

#include <kernel/qtree/request/reqattrlist.h>
#include <kernel/reqerror/reqerror.h>
#include <kernel/qtree/richrequest/nodeiterator.h>
#include <kernel/qtree/richrequest/printrichnode.h>
#include <kernel/qtree/richrequest/proxim.h>
#include <kernel/qtree/richrequest/richnode.h>

#include <library/cpp/charset/wide.h>
#include <library/cpp/getopt/opt.h>

#include <library/cpp/charset/recyr.hh>
#include <util/stream/file.h>
#include <util/stream/output.h>

#ifdef _win32_
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

void TestPhrases(TRichTreePtr& node, IOutputStream & Outf) {
    Outf << "TestPhrases" << Endl;
    TUserPhraseIterator it(node->Root);

    for (; !it.IsDone(); ++it)
        Outf << "  [Phrase]: " << PrintRichRequest(*it) << Endl;
}

void DeleteSecondWordInPhrase(TRichTreePtr& node, IOutputStream& Outf) {
    if (IsAndOp(*node->Root) && node->Root->Children.size() > 1) {
        node->Root->RemoveChildren(1, 2);
        Outf << "Without 2nd node: " << PrintRichRequest(node.Get()) << Endl;
    }
}

void DeleteFirstWordInPhrase(TRichTreePtr& node, IOutputStream& Outf) {
    if (IsAndOp(*node->Root) && node->Root->Children.size() > 1) {
        node->Root->RemoveChildren(0, 1);
        Outf << "Without 1st node: " << PrintRichRequest(node.Get()) << Endl;
    }
}

void DeleteLastWordInPhrase(TRichTreePtr& node, IOutputStream& Outf) {
    if (IsAndOp(*node->Root) && node->Root->Children.size() > 1) {
        node->Root->RemoveChildren(node->Root->Children.size() - 1, node->Root->Children.size());
        Outf << "Without last node: " << PrintRichRequest(node.Get()) << Endl;
    }
}

void DeleteThreeNodesWithRightProximity(TRichTreePtr& node, IOutputStream& Outf) {
    if (IsAndOp(*node->Root) && node->Root->Children.size() > 4) {
        node->Root->RemoveChildren(1, 4);
        Outf << "Without nodes 1, 2, 3 and right proximity: " << PrintRichRequest(node.Get()) << Endl;
    }
}

void DeleteThreeNodesWithLeftProximity(TRichTreePtr& node, IOutputStream& Outf) {
    if (IsAndOp(*node->Root) && node->Root->Children.size() > 3) {
        node->Root->RemoveChildren(3, 0);
        Outf << "Without nodes 1, 2, 3 and left proximity: " << PrintRichRequest(node.Get()) << Endl;
    }
}

void DeleteTwoFirstNodes(TRichTreePtr& node, IOutputStream& Outf) {
    if(IsAndOp(*node->Root) && node->Root->Children.size() > 2) {
        node->Root->RemoveChildren(0, 2);
        Outf << "Without two first nodes and right proximity: " << PrintRichRequest(node.Get()) << Endl;
    }
}

void DeleteTwoLastNodes(TRichTreePtr& node, IOutputStream& Outf) {
    if (IsAndOp(*node->Root) && node->Root->Children.size() > 2) {
        node->Root->RemoveChildren(node->Root->Children.size() - 2, node->Root->Children.size());
        Outf << "Without two last nodes and left proximity: " << PrintRichRequest(node.Get()) << Endl;
    }
}

void DeleteAllNodes(TRichTreePtr& node, IOutputStream& Outf) {
    if (IsAndOp(*node->Root)) {
        try {
            node->Root->RemoveChildren(0, node->Root->Children.size());
        } catch (const yexception& e) {
            Outf << "Delete all nodes: " << e.what() << Endl;
        }
    }
}

void TestRemoveChild(TRichTreePtr& node, IOutputStream& Outf) {
    TRichTreePtr node1(node->Copy());
    DeleteSecondWordInPhrase(node1, Outf);

    TRichTreePtr node2(node->Copy());
    DeleteFirstWordInPhrase(node2, Outf);

    TRichTreePtr node3(node->Copy());
    DeleteLastWordInPhrase(node3, Outf);

    TRichTreePtr node4(node->Copy());
    DeleteThreeNodesWithRightProximity(node4, Outf);

    TRichTreePtr node5(node->Copy());
    DeleteThreeNodesWithLeftProximity(node5, Outf);

    TRichTreePtr node6(node->Copy());
    DeleteTwoFirstNodes(node6, Outf);

    TRichTreePtr node7(node->Copy());
    DeleteTwoLastNodes(node7, Outf);

    TRichTreePtr node8(node->Copy());
    DeleteAllNodes(node8, Outf);
}

void TestReplaceChild(TRichTreePtr& node, IOutputStream& Outf) {
    if (IsAndOp(*node->Root) && node->Root->Children.size() > 0 && !IsMergedMark(*node->Root)) {
        TCreateTreeOptions opts(LI_DEFAULT_REQUEST_LANGUAGES);
        TRichNodePtr tree(CreateRichNode(u"(new && child)", opts));
        node->Root->ReplaceChild(0, tree);
        Outf << "With replaced 1st node: " << PrintRichRequest(node.Get()) << Endl;
    }
}

void TestAddRestriction(TRichTreePtr& node, IOutputStream& Outf) {
    TRichTreePtr zone(node->Copy());
    zone->Root->AddZoneFilter("title");
    Outf << "With oZone: " << PrintRichRequest(zone.Get()) << Endl;

    TRichTreePtr andnot(node->Copy());
    TCreateTreeOptions opts(LI_DEFAULT_REQUEST_LANGUAGES);
    TRichNodePtr restr = CreateRichNode(u"(a | b)", opts);
    andnot->Root->AddAndNotFilter(restr);
    Outf << "With oAndNot: " << PrintRichRequest(andnot.Get()) << Endl;

    TRichTreePtr restrdoc(node->Copy());
    TRichNodePtr dom = CreateRichNode(u"domain:\"wapbbs\"", opts);
    restrdoc->Root->AddRestrDocFilter(dom);
    Outf << "With oRestrDoc: " << PrintRichRequest(restrdoc.Get()) << Endl;

    TRichTreePtr refine(node->Copy());
    TRichNodePtr restriction = CreateRichNode(u"дерево", opts);
    refine->Root->AddRefineFilter(restriction);
    Outf << "With oRefine: " << PrintRichRequest(refine.Get()) << Endl;

    TRichTreePtr restrictbypos(node->Copy());
    restrictbypos->Root->AddRestrictByPosFilter(CreateEmptyRichNode());
    Outf << "With oRestrictByPos: " << PrintRichRequest(restrictbypos.Get()) << Endl;
}

typedef TTIterator<TReverseChildrenTraversal, AlwaysTrue> TRichNodeReverseIterator;
void TestReverseIterator(TRichNodePtr& node, IOutputStream& Outf) {
    TRichNodeReverseIterator it(node);
    Outf << "Reverse iterator: ";
    while (!it.IsDone()) {
        if (it.IsLast())
            Outf << "FirstNode: ";
        if (it->WordInfo.Get())
            Outf << (int)it->OpInfo.Op << ":" << WideToUTF8(it->WordInfo->GetNormalizedForm()) << " ";
        else
            Outf << (int)it->OpInfo.Op << ": ";
        ++it;
    }
    Outf << Endl;
}

void TestIterator(IInputStream* Inf, IOutputStream & Outf, ECharset codes) {
    TString req;
    TReqAttrList attrList("z:ZONE\na:ZONE\nb:ZONE\ndomain:ATTR_URL,docurl\nlink:ATTR_URL,zone\n"
        "title:ZONE\nhost:ATTR_URL,doc\nrhost:ATTR_URL,doc\nurl:ATTR_URL,doc\n"
        "anchor:ZONE");
    while (Inf->ReadLine(req)){
        Outf << req << '\n';
        if (req[0] != ':') {
            try {
                TLanguageContext lang(LI_DEFAULT_REQUEST_LANGUAGES, nullptr);
                TRichTreePtr richNode(CreateRichTree(CharToWide(req, codes), TCreateTreeOptions(lang, &attrList, RPF_ENABLE_EXTENDED_SYNTAX)));
                if (richNode.Get() && !req.empty()) {
                    TestPhrases(richNode, Outf);
                    TestRemoveChild(richNode, Outf);
                    TestReplaceChild(richNode, Outf);
                    TestAddRestriction(richNode, Outf);
                    TestReverseIterator(richNode->Root, Outf);
                    try {
                        richNode->Root->VerifyConsistency();
                    } catch (TCorruptedTreeException& e) {
                        Outf << "Error: VerifyConsistency() failed: " << e.what() << Endl;
                    }
                }
            } catch (const TError& e){
                Outf << Endl << e.what();
            }
            Outf << Endl;
        }
    }
}


int main(int argc, char* argv[]) {
#ifdef _win32_
    _setmode(_fileno(stdout), _O_BINARY);
#endif // _win32_

    try {
        const char* iname = nullptr;
        class Opt opt(argc, argv, "o:e:");
        int c = 0;
        ECharset codes = CODES_UTF8;

        while ((c = opt.Get()) != EOF) {
            switch (c){
                case 'o':
                    if (freopen(opt.Arg, "w", stderr) == nullptr) {
                        perror(opt.Arg);
                        return EXIT_FAILURE;
                    }
                    break;
                case 'e':
                    codes = CharsetByName(opt.Arg);
                    if (codes == CODES_UNKNOWN) {
                        perror(opt.Arg);
                        return EXIT_FAILURE;
                    }
                    break;
                case '?':
                default:
                    Cerr << "usage: nodeiter_test [-o errfile] [iname=test_req.txt [oname=out_req.txt]]" << Endl;
                    return EXIT_FAILURE;
            }
        }

        if (opt.Ind < argc){
            iname = argv[opt.Ind];
            opt.Ind++;
        }

        THolder<TFileInput> inf(iname ? new TFileInput(iname) : nullptr);

        if (opt.Ind < argc){
            TFixedBufferFileOutput outf(argv[opt.Ind]);
            TestIterator(inf.Get() ? inf.Get() : &Cin, outf, codes);
            opt.Ind++;
        } else {
            TestIterator(inf.Get() ? inf.Get() : &Cin, Cout, codes);
        }
        if (opt.Ind < argc) {
            Cerr << "extra arguments ignored: " << argv[opt.Ind] << "..." << Endl;
        }
        return 0;
    } catch (const std::exception& e) {
        Cerr << e.what() << Endl;
    }

    return 1;
}
