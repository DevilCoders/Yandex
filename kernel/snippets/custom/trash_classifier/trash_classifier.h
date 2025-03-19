#pragma once

#include <kernel/snippets/replace/replace.h>

#include <util/generic/strbuf.h>

namespace NSnippets {
    namespace NTrashClassifier {
        bool IsGoodEnough(const TWtringBuf& wtroka);
        bool IsTrash(const TReplaceContext& ctx, const TUtf16String& desc);
   }
}

