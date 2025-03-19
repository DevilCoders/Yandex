#pragma once

#include <kernel/snippets/iface/passagereplyfiller.h>

namespace NSnippets {

/////////////////////////////////////////////////////////////////////////////////////

class TPassageReplyFillerFactory : public IPassageReplyFillerFactory {
public:
    TAutoPtr<IPassageReplyFiller> Create(const TPassageReplyContext& ctx) override;
};

/////////////////////////////////////////////////////////////////////////////////////

REGISTER_PASSAGE_REPLY_FILLER_FACTORY(default, EPassageReplyFillerType::PRFT_DEFAULT, new TPassageReplyFillerFactory)

/////////////////////////////////////////////////////////////////////////////////////

} // namespace NSnippets
