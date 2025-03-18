#pragma once

#include "bounds.h"
#include "parsejson.h"

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>

namespace NSnippets {
    namespace NProto {
        class TSerpItem;
    }

    struct TUrlLink {
        TString Text = "";
        TString Url = "";

        TUrlLink(const TString& text, const TString& url)
          : Text(text)
          , Url(url)
        {
        }
    };

    TString FindQuery(const TSerpNodePtr& v);
    TSerpNodePtr FindNumFound(const TSerpNodePtr& v);
    bool ParseSerpItem(const TSerpNode& v, NProto::TSerpItem& res);
    void DumpSerpItem(const NProto::TSerpItem& s);
}
