package env

import (
	ydb_internal "a.yandex-team.ru/cloud/marketplace/license_server/internal/db/ydb"
	"a.yandex-team.ru/cloud/marketplace/pkg/auth"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/billing-private"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/marketplace-private"
)

type Backends struct {
	billing               billing.SessionManager
	marketplace           marketplace.SessionManager
	licenseServerProvider ydb_internal.LicenseServerProvider
	ycDefaultCredentials  auth.DefaultTokenAuth
}

func NewBackendsSet(options ...BackendsOption) *Backends {
	b := &Backends{}
	for _, init := range options {
		init(b)
	}

	return b
}

func (b *Backends) Billing() billing.SessionManager {
	return b.billing
}

func (b *Backends) Marketplace() marketplace.SessionManager {
	return b.marketplace
}

func (b *Backends) YDB() ydb_internal.LicenseServerProvider {
	return b.licenseServerProvider
}

func (b *Backends) YCDefaultTokenProvider() auth.DefaultTokenAuth {
	return b.ycDefaultCredentials
}

type BackendsOption func(b *Backends)

func BackendsWithYdb(licenseServerProvider ydb_internal.LicenseServerProvider) BackendsOption {
	return func(b *Backends) {
		b.licenseServerProvider = licenseServerProvider
	}
}

func BackendsWithBilling(billingClient billing.SessionManager) BackendsOption {
	return func(b *Backends) {
		b.billing = billingClient
	}
}

func BackendsWithMarketplace(marketplaceClient marketplace.SessionManager) BackendsOption {
	return func(b *Backends) {
		b.marketplace = marketplaceClient
	}
}

func BackendsWithYCDefaultCredentials(ycDefaltCredentials auth.DefaultTokenAuth) BackendsOption {
	return func(b *Backends) {
		b.ycDefaultCredentials = ycDefaltCredentials
	}
}
