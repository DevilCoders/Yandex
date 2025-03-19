#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {

class TRemoveEmojiReplacer : public IReplacer {
public:
    TRemoveEmojiReplacer()
        : IReplacer("remove_emoji")
    {
    }

    void DoWork(TReplaceManager* manager) override;
};

size_t CountEmojis(const TUtf16String& str);
bool RemoveEmojis(TUtf16String& str);
bool NeedRemoveEmojis(const TConfig& cfg, const TQueryy& query, const TString& url);
}
