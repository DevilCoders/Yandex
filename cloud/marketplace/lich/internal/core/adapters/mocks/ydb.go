package mocks

import (
	"context"

	"github.com/stretchr/testify/mock"

	"a.yandex-team.ru/cloud/marketplace/lich/internal/db/ydb"
	"a.yandex-team.ru/cloud/marketplace/pkg/monitoring/status"
)

type YDB struct {
	mock.Mock
}

func (d *YDB) Get(ctx context.Context, params ydb.GetProductVersionsParams) ([]ydb.ProductVersion, error) {
	args := d.Called(params)
	return args.Get(0).([]ydb.ProductVersion), args.Error(1)
}

func (d *YDB) Check(ctx context.Context) status.Status {
	return d.Called().Get(0).(status.Status)
}

func (d *YDB) Ping(ctx context.Context) error {
	return d.Called().Error(0)
}
