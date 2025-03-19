package pg

import (
	"context"
	"encoding/json"

	"github.com/jackc/pgtype"
	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/sqlerrors"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/internal/crypto"
)

var (
	querySelectFolderCoordsByClusterID = sqlutil.Stmt{
		Name: "SelectFolderCoordsByClusterID",
		Query: `
SELECT
	f.folder_ext_id AS folder_ext_id,
	cl.actual_rev AS revision
FROM
	dbaas.folders f
	JOIN dbaas.clusters cl USING (folder_id)
WHERE
	cl.cid = :cid
	AND code.match_visibility(cl, 'all')`,
	}

	querySelectClusterBySubclusterID = sqlutil.Stmt{
		Name:  "SelectClusterBySubclusterID",
		Query: "SELECT sc.cid FROM dbaas.subclusters sc WHERE sc.subcid = :subcid",
	}

	querySelectClusterByShardID = sqlutil.Stmt{
		Name: "SelectClusterByShardID",
		Query: `
SELECT sc.cid
FROM
    dbaas.shards s
    JOIN dbaas.subclusters sc USING (subcid)
WHERE
    s.shard_id = :shard_id`,
	}

	querySelectClusterByFQDN = sqlutil.Stmt{
		Name: "SelectClusterByFQDN",
		Query: `
SELECT sc.cid
FROM
    dbaas.hosts h
    JOIN dbaas.subclusters sc USING (subcid)
WHERE
    h.fqdn = :fqdn`,
	}

	querySelectPillarKey = sqlutil.Stmt{
		Name: "SelectPillarKey",
		Query: `
SELECT
    value#>:path as value
FROM
    dbaas.pillar
WHERE
    (:cid IS NULL OR cid = :cid)
    AND (:subcid IS NULL OR subcid = :subcid)
    AND (:shard_id IS NULL OR shard_id = :shard_id)
    AND (:fqdn IS NULL OR fqdn = :fqdn)`,
	}
)

func (b *Backend) FolderCoordsByClusterID(ctx context.Context, cid string) (string, int64, error) {
	var res struct {
		FolderExtID string `db:"folder_ext_id"`
		Revision    int64  `db:"revision"`
	}
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&res)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		querySelectFolderCoordsByClusterID,
		map[string]interface{}{
			"cid": cid,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return "", 0, err
	}
	if count == 0 {
		return "", 0, sqlerrors.ErrNotFound
	}

	return res.FolderExtID, res.Revision, nil
}

func (b *Backend) ClusterIDBySubClusterID(ctx context.Context, subcid string) (string, error) {
	var cid string
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&cid)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		querySelectClusterBySubclusterID,
		map[string]interface{}{
			"subcid": subcid,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return "", err
	}

	if count == 0 {
		return "", semerr.NotFoundf("cluster matching subcid %q not found", subcid)
	}

	return cid, nil
}

func (b *Backend) ClusterIDByShardID(ctx context.Context, shardid string) (string, error) {
	var cid string
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&cid)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		querySelectClusterByShardID,
		map[string]interface{}{
			"shard_id": shardid,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return "", err
	}

	if count == 0 {
		return "", semerr.NotFoundf("cluster matching shardID %q not found", shardid)
	}

	return cid, nil
}

func (b *Backend) ClusterIDByFQDN(ctx context.Context, fqdn string) (string, error) {
	var cid string
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&cid)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		querySelectClusterByFQDN,
		map[string]interface{}{
			"fqdn": fqdn,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return "", err
	}

	if count == 0 {
		return "", semerr.NotFoundf("cluster matching fqdn %q not found", fqdn)
	}
	return cid, nil
}

func (b *Backend) ClusterPillarKey(ctx context.Context, cid string, path []string) (crypto.CryptoKey, error) {
	return b.pillarKey(ctx, cid, "", "", "", path)
}

func (b *Backend) SubClusterPillarKey(ctx context.Context, subcid string, path []string) (crypto.CryptoKey, error) {
	return b.pillarKey(ctx, "", subcid, "", "", path)
}

func (b *Backend) ShardPillarKey(ctx context.Context, shardid string, path []string) (crypto.CryptoKey, error) {
	return b.pillarKey(ctx, "", "", shardid, "", path)
}

func (b *Backend) HostPillarKey(ctx context.Context, fqdn string, path []string) (crypto.CryptoKey, error) {
	return b.pillarKey(ctx, "", "", "", fqdn, path)
}

func (b *Backend) pillarKey(ctx context.Context, cid, subcid, shardid, fqdn string, path []string) (crypto.CryptoKey, error) {
	var value json.RawMessage
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&value)
	}

	var dbPath pgtype.TextArray
	if err := dbPath.Set(path); err != nil {
		return crypto.CryptoKey{}, err
	}

	_, err := sqlutil.QueryTx(ctx, querySelectPillarKey, map[string]interface{}{
		"cid":      stringToDB(cid),
		"subcid":   stringToDB(subcid),
		"shard_id": stringToDB(shardid),
		"fqdn":     stringToDB(fqdn),
		"path":     dbPath,
	}, parser, b.logger)
	if err != nil {
		return crypto.CryptoKey{}, err
	}

	b.logger.Errorf("got pillar key %+v", string(value))

	result := crypto.CryptoKey{EncryptionVersion: -1}
	if err := json.Unmarshal(value, &result); err != nil {
		return crypto.CryptoKey{}, err
	}

	if result.EncryptionVersion == -1 {
		return crypto.CryptoKey{}, semerr.FailedPrecondition("target path contain non encrypted data")
	}

	return result, nil
}

func stringToDB(val string) pgtype.Text {
	if val == "" {
		return pgtype.Text{Status: pgtype.Null}
	}

	return pgtype.Text{
		Status: pgtype.Present,
		String: val,
	}
}
