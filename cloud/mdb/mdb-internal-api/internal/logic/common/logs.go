package common

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/logs"
)

type Logs interface {
	Logs(
		ctx context.Context,
		cid string,
		st logs.ServiceType,
		limit int64,
		opts LogsOptions,
	) (logs []logs.Message, more bool, err error)

	Stream(
		ctx context.Context,
		cid string,
		st logs.ServiceType,
		opts LogsOptions,
	) (<-chan LogsBatch, error)
}

// LogsOptions holds optional arguments for Logs func
type LogsOptions struct {
	// ColumnFilter should not care about duplication. Implementation of logs retrieval procedure should handle the case.
	ColumnFilter []string
	FromTS       optional.Time
	ToTS         optional.Time
	Offset       optional.Int64
	Filter       []sqlfilter.Term
}

type LogsBatch struct {
	Logs []logs.Message
	Err  error
}
