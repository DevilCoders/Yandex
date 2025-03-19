package mocks

import (
	"context"

	"github.com/stretchr/testify/mock"

	"a.yandex-team.ru/cloud/marketplace/lich/internal/core/model"
)

type DBAdapter struct {
	mock.Mock
}

func (d *DBAdapter) GetProductVersions(ctx context.Context, ids ...string) ([]model.ProductVersion, error) {
	args := d.Called(ids)
	return args.Get(0).([]model.ProductVersion), args.Error(1)
}
