package env

import (
	"a.yandex-team.ru/cloud/marketplace/pkg/auth"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/compute"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/marketplace-private"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/operation"
	rm "a.yandex-team.ru/cloud/marketplace/pkg/clients/resource-manager"
)

type Backends struct {
	marketplace     marketplace.SessionManager
	computeImage    compute.ImageServiceClient
	operation       operation.OperationServiceClient
	resourceManager rm.CloudService

	ycDefaultCredentials auth.DefaultTokenAuth
}

func NewBackendsSet(options ...BackendsOption) *Backends {
	b := &Backends{}
	for _, init := range options {
		init(b)
	}

	return b
}

func (b *Backends) Marketplace() marketplace.SessionManager {
	return b.marketplace
}

func (b *Backends) ResourceManager() rm.CloudService {
	return b.resourceManager
}

func (b *Backends) Operation() operation.OperationServiceClient {
	return b.operation
}

func (b *Backends) ComputeImage() compute.ImageServiceClient {
	return b.computeImage
}

func (b *Backends) YCDefaultTokenProvider() auth.DefaultTokenAuth {
	return b.ycDefaultCredentials
}

type BackendsOption func(b *Backends)

func BackendsWithMarketplace(marketplaceClient marketplace.SessionManager) BackendsOption {
	return func(b *Backends) {
		b.marketplace = marketplaceClient
	}
}

func BackendsWithComputeImage(computeImage compute.ImageServiceClient) BackendsOption {
	return func(b *Backends) {
		b.computeImage = computeImage
	}
}

func BackendsWithOperation(operation operation.OperationServiceClient) BackendsOption {
	return func(b *Backends) {
		b.operation = operation
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
