#pragma once

#include "bounds.h"
#include "img.h"

#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/generic/ptr.h>

namespace NJson {
    class TJsonValue;
}

namespace NSnippets {
    struct TSerpNode;
    typedef TSimpleSharedPtr<TSerpNode> TSerpNodePtr;
    typedef TVector<TSerpNodePtr> TSerpNodePtrs;
    struct TSerpNode {
        TString Class;
        TString Href;
        TString Src;
        TString Tag;
        TString Name;
        TString Value;
        TBounds Bounds;
        TSerpNodePtrs Children;
        TString InnerText;

        bool HasClass(TStringBuf x) const;
    };
    struct TSerp {
        TSerpNodePtr Layout;
        TString PngImage;
        TString Query;
        TSerpNodePtr NumFound;

        TCanvasPtr Image;
    };

    bool ParseSerp(const NJson::TJsonValue& v, TSerp& res);
    inline void AddImage(TSerp& res) {
        res.Image = TCanvas::FromBase64(res.PngImage);
    }
}
