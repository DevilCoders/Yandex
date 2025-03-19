package pg

import (
	"context"
	"database/sql"
	"encoding/json"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-config/internal/metadb"
)

var (
	queryConfigHostAuth = sqlutil.Stmt{
		Name: "ConfigHostAuth",
		Query: `
SELECT
    access_secret, type
FROM
    dbaas.config_host_access_ids
WHERE
    access_id = :access_id
    AND active = true
`,
	}

	queryGetManagedConfig = sqlutil.Stmt{
		Name:  "GetManagedConfig",
		Query: "SELECT code.get_managed_config(:fqdn, :target_id, :revision);",
	}

	queryGetUnmanagedConfig = sqlutil.Stmt{
		Name:  "GetUnmanagedConfig",
		Query: "SELECT code.get_unmanaged_config(:fqdn, :target_id, :revision);",
	}
)

func (b *Backend) ConfigHostAuth(ctx context.Context, accessID string) (metadb.ConfigHostAuthInfo, error) {
	result := metadb.ConfigHostAuthInfo{}
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&result)
	}
	count, err := sqlutil.QueryTx(
		ctx,
		queryConfigHostAuth,
		map[string]interface{}{
			"access_id": accessID,
		},
		parser,
		b.logger,
	)

	if err != nil {
		return metadb.ConfigHostAuthInfo{}, err
	}

	// access_id is primary key, multiple result shouldn't exist
	if count != 1 {
		return metadb.ConfigHostAuthInfo{}, semerr.Authentication("Access ID not found")
	}

	return result, nil
}

func (b *Backend) GenerateManagedConfig(ctx context.Context, fqdn string, targetPillarID string, revision optional.Int64) (json.RawMessage, error) {
	var pillar json.RawMessage
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&pillar)
	}

	_, err := sqlutil.QueryTx(
		ctx,
		queryGetManagedConfig,
		map[string]interface{}{
			"fqdn":      fqdn,
			"target_id": sql.NullString{String: targetPillarID, Valid: targetPillarID != ""},
			"revision":  sql.NullInt64{Int64: revision.Int64, Valid: revision.Valid},
		},
		parser,
		b.logger,
	)
	if err != nil {
		return nil, err
	}

	return pillar, nil
}

func (b *Backend) GenerateUnmanagedConfig(ctx context.Context, fqdn string, targetPillarID string, revision optional.Int64) (json.RawMessage, error) {
	var pillar json.RawMessage
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&pillar)
	}

	_, err := sqlutil.QueryTx(
		ctx,
		queryGetUnmanagedConfig,
		map[string]interface{}{
			"fqdn":      fqdn,
			"target_id": sql.NullString{String: targetPillarID, Valid: targetPillarID != ""},
			"revision":  sql.NullInt64{Int64: revision.Int64, Valid: revision.Valid},
		},
		parser,
		b.logger,
	)
	if err != nil {
		return nil, err
	}

	return pillar, nil
}
