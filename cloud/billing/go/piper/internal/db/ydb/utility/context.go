package utility

import (
	"context"
	"database/sql"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
)

func (q *Queries) GetContext(ctx context.Context, namespace string) (result []ContextRow, err error) {
	ctx = tooling.QueryStarted(ctx)
	defer func() {
		tooling.QueryDone(ctx, err)
	}()

	nsParam := ydb.UTF8Value(namespace)

	query := getContextQuery.WithParams(q.qp)
	err = q.db.SelectContext(ctx, &result, query, sql.Named("namespace", nsParam))
	err = qtool.WrapWithQuery(err, query)
	return
}

type ContextRow struct {
	Key   string `db:"key"`
	Value string `db:"value"`
}

var getContextQuery = qtool.Query(
	qtool.Declare("namespace", ydb.TypeUTF8),
	"SELECT", qtool.PrefixedCols("context", "key", "value"),
	"FROM", qtool.TableAs("utility/context", "context"),
	`WHERE key like $namespace || "%"`,
)
