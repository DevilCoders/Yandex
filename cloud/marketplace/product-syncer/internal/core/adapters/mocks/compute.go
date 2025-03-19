package mocks

import (
	"a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/compute/v1"
	"a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/operation"
	"context"

	"github.com/stretchr/testify/mock"
)

type ImageService struct {
	mock.Mock
}

func (c *ImageService) Create(ctx context.Context, in *compute.CreateImageRequest) (*operation.Operation, error) {
	args := c.Called(in)
	return args.Get(0).(*operation.Operation), args.Error(1)
}
