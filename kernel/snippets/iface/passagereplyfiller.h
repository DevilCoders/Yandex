#pragma once

#include <kernel/snippets/idl/enums.h>

#include "replycontext.h"

namespace NSnippets {

/////////////////////////////////////////////////////////////////////////////////////

class IPassageReplyFiller {
public:
    virtual ~IPassageReplyFiller() = default;

    virtual void FillPassageReply(TPassageReply& reply) = 0;
};

/////////////////////////////////////////////////////////////////////////////////////

class IPassageReplyFillerFactory {
public:
    virtual ~IPassageReplyFillerFactory() = default;

    virtual TAutoPtr<IPassageReplyFiller> Create(const TPassageReplyContext& ctx) = 0;
};

/////////////////////////////////////////////////////////////////////////////////////

void SetPassageReplyFillerFactory(EPassageReplyFillerType type, TAutoPtr<IPassageReplyFillerFactory> factory);

TAutoPtr<IPassageReplyFiller> CreatePassageReplyFiller(const TPassageReplyContext& ctx);

/////////////////////////////////////////////////////////////////////////////////////

class TPassageReplyFillerFactoryRegistrar {
public:
    TPassageReplyFillerFactoryRegistrar(EPassageReplyFillerType type, TAutoPtr<IPassageReplyFillerFactory> factory)
    {
        SetPassageReplyFillerFactory(type, factory);
    }
};

#define REGISTER_PASSAGE_REPLY_FILLER_FACTORY(name, type, factory) \
static const NSnippets::TPassageReplyFillerFactoryRegistrar passageReplyFillerFactoryRegistrar_##name(type, factory);

/////////////////////////////////////////////////////////////////////////////////////

} // namespace NSnippets
