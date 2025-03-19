package meta

import (
	"context"
	"database/sql"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
)

func (q *Queries) GetSchemasByName(ctx context.Context, names ...string) (result []SchemaRow, err error) {
	ctx = tooling.QueryStarted(ctx)
	defer func() {
		tooling.QueryDone(ctx, err)
	}()

	param := makeStringList(names)
	query := getSchemasByNamesQuery.WithParams(q.qp)
	err = q.db.SelectContext(ctx, &result, query, sql.Named("names", param))
	err = qtool.WrapWithQuery(err, query)
	return
}

func (q *Queries) GetAllSchemas(ctx context.Context) (result []SchemaRow, err error) {
	ctx = tooling.QueryStarted(ctx)
	defer func() {
		tooling.QueryDone(ctx, err)
	}()

	query := getAllSchemasQuery.WithParams(q.qp)
	err = q.db.SelectContext(ctx, &result, query)
	err = qtool.WrapWithQuery(err, query)
	return
}

type SchemaRow struct {
	ServiceID string             `db:"service_id"`
	Name      string             `db:"name"`
	Tags      qtool.JSONAnything `db:"tags"`
}

var (
	getSchemasByNamesQuery = qtool.Query(
		qtool.Declare("names", stringList),
		"SELECT", qtool.PrefixedCols("schemas", "service_id", "name", "tags"),
		"FROM", qtool.TableAs("meta/schemas", "schemas"),
		"WHERE schemas.name in $names",
	)
	getAllSchemasQuery = qtool.Query(
		"SELECT", qtool.Cols("service_id", "name", "tags"),
		"FROM", qtool.Table("meta/schemas"),
	)
)
