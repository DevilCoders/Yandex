package usage

import (
	"context"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
)

type YDB interface {
	ExecuteSchemeQuery(ctx context.Context, query string, opts ...table.ExecuteSchemeQueryOption) error
}

func NewScheme(db YDB, params qtool.QueryParams) *Schemes {
	return &Schemes{db: db, qp: params}
}

type Schemes struct {
	db YDB
	qp qtool.QueryParams
}
