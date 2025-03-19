#include "schemaorg_serializer.h"
#include <kernel/snippets/schemaorg/proto/schemaorg.pb.h>

#include <util/generic/buffer.h>
#include <util/stream/buffer.h>
#include <util/stream/str.h>
#include <util/stream/zlib.h>
#include <library/cpp/string_utils/base64/base64.h>

namespace NSchemaOrg {
    TString SerializeToBase64(const TTreeNode& tree) {
        if (tree.NodeSize() == 0) {
            return TString();
        }
        TBuffer compressed;
        {
            TBufferOutput out(compressed);
            {
                TZLibCompress compress(&out);
                tree.SerializeToArcadiaStream(&compress);
                compress.Finish();
            }
        }
        return Base64Encode(TStringBuf(compressed.data(), compressed.size()));
    }

    bool DeserializeFromBase64(const TStringBuf& base64Str, TTreeNode& result) {
        TString str = Base64Decode(base64Str);
        TStringInput input(str);
        TZLibDecompress decompress(&input);
        return result.ParseFromArcadiaStream(&decompress);
    }

} // namespace NSchemaOrg
