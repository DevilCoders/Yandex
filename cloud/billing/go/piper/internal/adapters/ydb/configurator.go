package ydb

import (
	"context"
	"strings"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/utility"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
)

type Configurator struct {
	retrier

	db          *sqlx.DB
	queryParams qtool.QueryParams
}

func NewConfigurator(runCtx context.Context, db *sqlx.DB, rootPath string) *Configurator {
	if rootPath != "" && !strings.HasSuffix(rootPath, "/") {
		rootPath = rootPath + "/"
	}
	return &Configurator{
		db:          db,
		queryParams: qtool.QueryParams{RootPath: rootPath},
	}
}

func (c *Configurator) GetConfig(ctx context.Context, namespace string) (result map[string]string, err error) {
	var dbContext []utility.ContextRow

	nsPrefix := namespace + "."
	{
		retryCtx := tooling.StartRetry(ctx)
		err = c.retryRead(retryCtx, func() (resErr error) {
			tooling.RetryIteration(retryCtx)

			tx, err := c.db.BeginTxx(retryCtx, readCommitted())
			if err != nil {
				return err
			}
			defer func() {
				autoTx(tx, resErr)
			}()

			queries := utility.New(tx, c.queryParams)
			dbContext, err = queries.GetContext(retryCtx, nsPrefix)
			return err
		})
	}
	if err != nil {
		return nil, err
	}

	result = make(map[string]string, len(dbContext))
	for _, row := range dbContext {
		key := strings.TrimPrefix(row.Key, nsPrefix)
		result[key] = row.Value
	}
	return result, err
}
