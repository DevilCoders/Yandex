package metadb

import (
	"context"
	"encoding/json"
	"io"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
)

//go:generate ../../../scripts/mockgen.sh MetaDB

type MetaDB interface {
	io.Closer
	ready.Checker

	sqlutil.TxBinder

	ConfigHostAuth(ctx context.Context, accessID string) (ConfigHostAuthInfo, error)

	GenerateManagedConfig(ctx context.Context, fqdn string, targetPillarID string, revision optional.Int64) (json.RawMessage, error)
	GenerateUnmanagedConfig(ctx context.Context, fqdn string, targetPillarID string, revision optional.Int64) (json.RawMessage, error)
}

type ConfigHostAuthInfo struct {
	AccessSecret string `db:"access_secret"`
	AccessType   string `db:"type"`
}
