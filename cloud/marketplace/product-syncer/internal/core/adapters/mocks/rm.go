package mocks

import (
	"context"

	"github.com/stretchr/testify/mock"
)

type CloudService struct {
	mock.Mock
}

func (c *CloudService) GetPermissionStages(ctx context.Context, cloudID string) ([]string, error) {
	args := c.Called(cloudID)
	return args.Get(0).([]string), args.Error(1)
}
