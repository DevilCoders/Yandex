package ydb

import (
	"context"
	"time"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
)

//go:generate mockery --inpackage --testonly --disable-version-string --name SchemeSession

type SchemeSessionGetter interface {
	Get(context.Context) (SchemeSession, error)
	Put(context.Context, SchemeSession) error
}

type SchemeSession interface {
	ExecuteSchemeQuery(ctx context.Context, query string, opts ...table.ExecuteSchemeQueryOption) error
	DescribeTable(ctx context.Context, path string, opts ...table.DescribeTableOption) (desc table.Description, err error)
}

type YDBPool interface {
	Get(context.Context) (*table.Session, error)
	Put(context.Context, *table.Session) error
}

func YDBScheme(p YDBPool) SchemeSessionGetter {
	getter := ydbWrapper{p}
	return getter
}

type ydbWrapper struct {
	YDBPool
}

func (w ydbWrapper) Get(ctx context.Context) (sess SchemeSession, err error) {
	sess, err = w.YDBPool.Get(ctx)
	return
}

func (w ydbWrapper) Put(ctx context.Context, s SchemeSession) error {
	err := w.YDBPool.Put(ctx, s.(*table.Session))
	return err
}

// ydbDate converts time to ydb date skiping timezone,
// in ydb date is (time-epoch)/24h and with Mooscow timezone it will be previous day.
func ydbDate(d time.Time) time.Time {
	return time.Date(d.Year(), d.Month(), d.Day(), 0, 0, 0, 0, time.UTC)
}
