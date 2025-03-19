#include "passagereplyfiller.h"

#include <kernel/snippets/idl/snippets.pb.h>

#include <util/generic/singleton.h>

namespace NSnippets {

/////////////////////////////////////////////////////////////////////////////////////

class TPassageReplyFillerFactoriesHolder {
public:
    TPassageReplyFillerFactoriesHolder()
        : Factories(EPassageReplyFillerType::PRFT_COUNT)
    {}

    void SetFactory(EPassageReplyFillerType type, TAutoPtr<IPassageReplyFillerFactory> factory)
    {
        Factories[type].Reset(factory.Release());
    }

    IPassageReplyFillerFactory* GetFactory(EPassageReplyFillerType type) const
    {
        return Factories[type].Get();
    }

private:
    TVector<TAutoPtr<IPassageReplyFillerFactory>> Factories;
};

/////////////////////////////////////////////////////////////////////////////////////

void SetPassageReplyFillerFactory(EPassageReplyFillerType type, TAutoPtr<IPassageReplyFillerFactory> factory)
{
    Singleton<TPassageReplyFillerFactoriesHolder>()->SetFactory(type, factory);
}

TAutoPtr<IPassageReplyFiller> CreatePassageReplyFiller(const TPassageReplyContext& ctx)
{
    EPassageReplyFillerType factoryType = EPassageReplyFillerType::PRFT_DEFAULT;
    if (nullptr != ctx.ConfigParams.SRP && ctx.ConfigParams.SRP->HasPassageReplyFillerType()) {
        const ui32 type{ctx.ConfigParams.SRP->GetPassageReplyFillerType()};

        Y_VERIFY(type < EPassageReplyFillerType::PRFT_COUNT, "Invalid PassageReplyFillerType in TSnipReqParams");

        factoryType = static_cast<EPassageReplyFillerType>(type);
    }

    IPassageReplyFillerFactory* currentFactory = Singleton<TPassageReplyFillerFactoriesHolder>()->GetFactory(factoryType);
    Y_VERIFY(currentFactory != nullptr);
    return currentFactory->Create(ctx);
}

/////////////////////////////////////////////////////////////////////////////////////

} // namespace NSnippets
