#include "wizserp.h"

#include <tools/snipmake/common/nohtml.h>
#include <library/cpp/cgiparam/cgiparam.h>

namespace NSnippets {

    TWizReqIterator::TWizReqIterator(IInputStream* in)
        : Stream(in, ZLib::Auto)
        , Index(0)
    {
    }
    TRichTreePtr TWizReqIterator::RichTree()const {
        return Tree;
    }
    TString TWizReqIterator::GetB64QTree() const {
        return B64QTree;
    }
    const TString& TWizReqIterator::Request()const {
        return Req;
    }
    const TString& TWizReqIterator::Region() const {
        return Reg;
    }
    bool TWizReqIterator::Next() {
        Str.clear();
        Req.clear();
        Reg.clear();
        Tree.Drop();
        B64QTree.clear();
        ++Index;

        if (Stream.ReadTo(Str,'\t') && Stream.ReadTo(Req,'\t') && Stream.ReadTo(Str,'\t') && Stream.ReadTo(Reg, '\n')) {
            TCgiParameters pars;
            pars.Scan(Str.c_str());

            if (pars.NumOfValues("qtree")) {
                Str = pars.Get("qtree");
                try {
                    Tree = DeserializeRichTree(DecodeRichTreeBase64(Str));
                } catch (yexception) {
                    Cerr << "Broken qtree: " << Str << Endl;
                }
                B64QTree = Str;
                Req = HtmlEscape(Req);
            }
            return true;
        } else
            Stream.ReadLine(Str);
        return false;
    }

    TSerpXmlIterator::TSerpXmlIterator(IInputStream* serp, IInputStream* wiz)
        : Serp(serp)
        , Trees(wiz)
        , State(0)
    {
    }

    const TReqSerp& TSerpXmlIterator::Get() const {
        return Current;
    }

    bool TSerpXmlIterator::IsSerpValid(const TReqSerp* serp) const {
        return Trees.Region() == serp->Region && Trees.Request() == serp->Query;
    }
    bool TSerpXmlIterator::Next() {
        const TReqSerp* serp = nullptr;
        if (!Trees.Next())
            return false;
        do {
            if (!Serp.Next())
                return false;
            serp = &Serp.Get();
        } while (!IsSerpValid(serp)); // пропускаем серпы без переколдовок

        Current = *serp;
        Current.RichRequestTree = Trees.RichTree();
        Current.B64QTree = Trees.GetB64QTree();
        for (auto& snip : Current.Snippets) {
            snip.RichRequestTree = Current.RichRequestTree;
            snip.B64QTree = Current.B64QTree;
        }
        return true;
    }

}
