package meta

import (
	"context"
	"database/sql"
	"time"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
)

func (q *Queries) GetBAMappingByID(ctx context.Context, id ...string) (result []BillingAccountMappingRow, err error) {
	ctx = tooling.QueryStarted(ctx)
	tooling.QueryWithRowsCount(ctx, len(id))

	defer func() {
		tooling.QueryDone(ctx, err)
	}()

	param := makeStringList(id)
	query := getBAMappingQuery.WithParams(q.qp)
	err = q.db.SelectContext(ctx, &result, query, sql.Named("ids", param))
	err = qtool.WrapWithQuery(err, query)
	return
}

func (q *Queries) GetBABindings(ctx context.Context, instanceType string, from, to time.Time, ids ...string) (
	result []InstanceBindingRow, err error,
) {
	ctx = tooling.QueryStarted(ctx)
	tooling.QueryWithRowsCount(ctx, len(ids))

	defer func() {
		tooling.QueryDone(ctx, err)
	}()

	idsParam := makeStringList(ids)
	typeParam := ydb.UTF8Value(instanceType)
	fromParam := qtool.UInt64Ts(from).Value()
	toParam := qtool.UInt64Ts(to).Value()
	query := getBABindingsQuery.WithParams(q.qp)
	err = q.db.SelectContext(ctx, &result, query,
		sql.Named("ids", idsParam),
		sql.Named("instance_type", typeParam),
		sql.Named("from", fromParam),
		sql.Named("to", toParam),
	)
	err = qtool.WrapWithQuery(err, query)
	return
}

type BillingAccountMappingRow struct {
	ID              string       `db:"id"`
	MasterAccountID qtool.String `db:"master_account_id"`
}

var getBAMappingQuery = qtool.Query(
	qtool.Declare("ids", stringList),
	"SELECT", qtool.PrefixedCols("ba", "id", "master_account_id"),
	"FROM", qtool.TableAs("meta/billing_accounts", "ba"),
	"WHERE ba.id in $ids;",
)

type InstanceBindingRow struct {
	ServiceInstanceType string         `db:"service_instance_type"`
	ServiceInstanceID   string         `db:"service_instance_id"`
	BillingAccountID    string         `db:"billing_account_id"`
	EffectiveTime       qtool.UInt64Ts `db:"effective_time"`
	EffectiveTo         qtool.UInt64Ts `db:"effective_to"`
}

var getBABindingsQuery = qtool.Query(
	qtool.Declare("ids", stringList),
	qtool.Declare("instance_type", ydb.TypeUTF8),
	qtool.Declare("from", ydb.TypeUint64),
	qtool.Declare("to", ydb.TypeUint64),
	qtool.InfTSDecl,

	qtool.NamedQuery("filtered",
		"SELECT ", qtool.Cols(
			qtool.PrefixedCols("ib", "service_instance_type", "service_instance_id", "billing_account_id", "effective_time"),
			"NVL(LEAD(ib.effective_time) OVER ew, $inf_ts) as effective_to",
		),
		"FROM", qtool.TableAs("meta/service_instance_bindings/bindings", "ib"),
		"WHERE ib.service_instance_type = $instance_type AND ib.service_instance_id in $ids",
		"WINDOW ew AS (PARTITION BY ib.service_instance_type, ib.service_instance_id order by ib.effective_time)",
	),

	"SELECT", qtool.Cols("service_instance_type", "service_instance_id", "billing_account_id", "effective_time", "effective_to"),
	"FROM $filtered as ib",
	"WHERE effective_to > $from AND ib.effective_time <= $to",
	"AND billing_account_id IS NOT NULL",
)
