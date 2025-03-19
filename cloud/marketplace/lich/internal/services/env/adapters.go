package env

import (
	"a.yandex-team.ru/cloud/marketplace/lich/internal/core/adapters"
	"a.yandex-team.ru/cloud/marketplace/lich/internal/core/adapters/db"
	"a.yandex-team.ru/cloud/marketplace/lich/internal/core/adapters/db/ydb"
)

type Adapters struct {
	env *Env
}

func (env *Env) Adapters() *Adapters {
	return &Adapters{
		env: env,
	}
}

func (a *Adapters) Billing() adapters.BillingAdapter {
	return adapters.NewBillingAdapter(
		a.env.Backends().Billing(),
		a.env.Backends().YCDefaultTokenProvider(),
	)
}

func (a *Adapters) DB() db.ProductVersionsAdapter {
	return ydb.NewProductVersionAdapter(a.env.Backends().YDB())
}
