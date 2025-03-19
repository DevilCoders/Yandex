#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NSchemaOrg {
    class TTreeNode;

    TString SerializeToBase64(const TTreeNode& tree);

    bool DeserializeFromBase64(const TStringBuf& base64Str, TTreeNode& result);

} // namespace NSchemaOrg
