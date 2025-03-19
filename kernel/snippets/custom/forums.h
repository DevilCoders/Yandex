#pragma once

#include <kernel/snippets/custom/forums_handler/forums_handler.h>
#include <kernel/snippets/replace/replace.h>

#include <util/generic/ptr.h>

namespace NSnippets
{

class TForumReplacer: public IReplacer {
private:
    const TForumMarkupViewer& ForumViewer;

public:
    explicit TForumReplacer(const TForumMarkupViewer& forumViewer)
      : IReplacer("forum")
      , ForumViewer(forumViewer)
    {
    }

    void DoWork(TReplaceManager* manager) override;
};

}
