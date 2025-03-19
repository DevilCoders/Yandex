package dns

import (
	"context"

	"a.yandex-team.ru/library/go/core/xerrors"
)

// Errors
var (
	ErrRequestFailed = xerrors.NewSentinel("request failed")
)

// Rec content of DNS record
type Rec struct {
	Value string
	TTL   uint
}

// MapRec List of DNS records
type MapRec map[string]Rec

// Client base interface for compute DNS API
type Client interface {
	SetCNAMERecord(ctx context.Context, netid, name, value string) error
	DeleteCNAMERecord(ctx context.Context, netid, name string) error
	ListRecords(ctx context.Context, netid string) (MapRec, error)
}
