package env

import (
	"a.yandex-team.ru/cloud/marketplace/pkg/auth"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/billing-private"

	"a.yandex-team.ru/cloud/marketplace/lich/internal/db/ydb"

	rm "a.yandex-team.ru/cloud/marketplace/pkg/clients/resource-manager"
)

type Backends struct {
	billing         billing.SessionManager
	resourceManager rm.CloudService

	productVersionProvider ydb.ProductVersionsProvider

	ycDefaultCredentials auth.DefaultTokenAuth
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

func (b *Backends) YDB() ydb.ProductVersionsProvider {
	return b.productVersionProvider
}

func (b *Backends) ResourceManager() rm.CloudService {
	return b.resourceManager
}

func (b *Backends) YCDefaultTokenProvider() auth.DefaultTokenAuth {
	return b.ycDefaultCredentials
}

type BackendsOption func(b *Backends)

func BackendsWithBilling(billingClient billing.SessionManager) BackendsOption {
	return func(b *Backends) {
		b.billing = billingClient
	}
}

func BackendsWithYdb(productVersionProvider ydb.ProductVersionsProvider) BackendsOption {
	return func(b *Backends) {
		b.productVersionProvider = productVersionProvider
	}
}

func BackendsWithResourceManager(resourceManager rm.CloudService) BackendsOption {
	return func(b *Backends) {
		b.resourceManager = resourceManager
	}
}

func BackendsWithYCDefaultCredentials(ycDefaltCredentials auth.DefaultTokenAuth) BackendsOption {
	return func(b *Backends) {
		b.ycDefaultCredentials = ycDefaltCredentials
	}
}
