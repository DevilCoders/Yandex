#include "builder.h"
#include "numhandler.h"
#include "treedata.h"

#include <library/cpp/html/html5/parse.h>
#include <library/cpp/html/storage/storage.h>
#include <library/cpp/html/face/propface.h>

#include <util/stream/input.h>

namespace NDomTree {
    TDomTreePtr BuildTree(IInputStream& htmlStream, const TString& url, const TString& charset,
                          size_t maxAllowedTreeDepth) {
        TString html = htmlStream.ReadAll();
        return BuildTree(html, url, charset, maxAllowedTreeDepth);
    }

    TDomTreePtr BuildTree(const TStringBuf& html, const TString& url, const TString& charset,
                          size_t maxAllowedTreeDepth) {
        NHtml::TStorage storage;
        {
            NHtml::TParserResult parsed(storage);
            NHtml5::ParseHtml(html, &parsed);
        }

        THolder<IParsedDocProperties> props(CreateParsedDocProperties());
        props->SetProperty(PP_BASE, url.data());
        props->SetProperty(PP_CHARSET, charset.data());

        auto handlerPtr = NDomTree::TreeBuildingHandler();
        Numerator numerator(*handlerPtr);
        numerator.Numerate(storage.Begin(), storage.End(), props.Get(), nullptr);

        if (handlerPtr->GetMaxDepth() <= maxAllowedTreeDepth) {
            return handlerPtr->GetTree();
        } else {
            return CreateTreeBuilder();
        }
    }

}
