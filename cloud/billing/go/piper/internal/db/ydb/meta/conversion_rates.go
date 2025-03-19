package meta

import (
	"context"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
)

func (q *Queries) GetAllConversionRates(ctx context.Context) (result []ConversionRateRow, err error) {
	ctx = tooling.QueryStarted(ctx)
	defer func() {
		tooling.QueryDone(ctx, err)
	}()

	query := getAllConversionRatesQuery.WithParams(q.qp)
	err = q.db.SelectContext(ctx, &result, query)
	err = qtool.WrapWithQuery(err, query)
	return
}

type ConversionRateRow struct {
	SourceCurrency string               `db:"source_currency"`
	TargetCurrency string               `db:"target_currency"`
	EffectiveTime  qtool.UInt64Ts       `db:"effective_time"`
	Multiplier     qtool.DefaultDecimal `db:"multiplier"`
}

var getAllConversionRatesQuery = qtool.Query(
	"SELECT", qtool.Cols("source_currency", "target_currency", "effective_time", "multiplier"),
	"FROM", qtool.Table("utility/conversion_rates"),
)
