#pragma once

#include <kernel/snippets/replace/replace.h>

#include <util/generic/string.h>

namespace NSnippets {

    class TFactSnipReplacer : public IReplacer {
        public:
            TFactSnipReplacer();
            void DoWork(TReplaceManager* manager) override;
    };

} // namespace NSnippets
