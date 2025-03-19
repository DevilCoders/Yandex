package mocks

import (
	"context"

	"github.com/stretchr/testify/mock"

	"a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/operation"
)

type OperationService struct {
	mock.Mock
}

func (c *OperationService) Get(ctx context.Context, opID string) (*operation.Operation, error) {
	args := c.Called(opID)
	return args.Get(0).(*operation.Operation), args.Error(1)
}
