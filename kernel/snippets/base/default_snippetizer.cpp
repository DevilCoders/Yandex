#include "default_snippetizer.h"

#include "basesnip.h"

namespace NSnippets {

/////////////////////////////////////////////////////////////////////////////////////

class TPassageReplyFiller : public IPassageReplyFiller {
public:
    TPassageReplyFiller(const TPassageReplyContext& ctx)
        : Context(ctx)
    {}

    void FillPassageReply(TPassageReply& reply) override
    {
        NSnippets::FillPassageReply(reply, Context);
    }

private:
    const TPassageReplyContext& Context;
};

/////////////////////////////////////////////////////////////////////////////////////

TAutoPtr<IPassageReplyFiller> TPassageReplyFillerFactory::Create(const TPassageReplyContext& ctx)
{
    return new TPassageReplyFiller(ctx);
}

/////////////////////////////////////////////////////////////////////////////////////

} // namespace NSnippets
