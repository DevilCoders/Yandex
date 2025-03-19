package env

import "a.yandex-team.ru/cloud/marketplace/product-syncer/internal/core/adapters"

type Adapters struct {
	env *Env
}

func (env *Env) Adapters() *Adapters {
	return &Adapters{
		env: env,
	}
}

func (a *Adapters) Marketplace() adapters.MarketplaceAdapter {
	return adapters.NewMarketplaceAdapter(
		a.env.Backends().Marketplace(),
		a.env.Backends().YCDefaultTokenProvider(),
	)
}
