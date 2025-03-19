package env

import (
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/adapters/billing"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/adapters/db"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/adapters/db/ydb"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/adapters/marketplace"
)

type Adapters struct {
	env *Env
}

func (env *Env) Adapters() *Adapters {
	return &Adapters{
		env: env,
	}
}

func (a *Adapters) Billing() billing.BillingAdapter {
	return billing.NewBillingAdapter(
		a.env.Backends().Billing(),
		a.env.Backends().YCDefaultTokenProvider(),
	)
}

func (a *Adapters) Marketplace() marketplace.MarketplaceAdapter {
	return marketplace.NewMarketplaceAdapter(
		a.env.Backends().Marketplace(),
		a.env.Backends().YCDefaultTokenProvider(),
	)
}

func (a *Adapters) DB() db.LicenseServerAdapter {
	return ydb.NewLicenseServerAdapter(a.env.Backends().YDB())
}
