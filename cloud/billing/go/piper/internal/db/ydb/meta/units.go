package meta

import (
	"context"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
)

func (q *Queries) GetAllUnits(ctx context.Context) (result []UnitRow, err error) {
	ctx = tooling.QueryStarted(ctx)
	defer func() {
		tooling.QueryDone(ctx, err)
	}()

	query := getAllUnitsQuery.WithParams(q.qp)
	err = q.db.SelectContext(ctx, &result, query)
	err = qtool.WrapWithQuery(err, query)
	return
}

type UnitRow struct {
	SrcUnit string `db:"src_unit"`
	DstUnit string `db:"dst_unit"`
	Factor  uint64 `db:"factor"`
	Reverse bool   `db:"reverse"`
	Type    string `db:"type"`
}

var getAllUnitsQuery = qtool.Query(
	"SELECT", qtool.Cols("src_unit", "dst_unit", "factor", "reverse", "type"),
	"FROM", qtool.Table("meta/units"),
)
