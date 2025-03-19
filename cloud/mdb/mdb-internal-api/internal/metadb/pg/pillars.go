package pg

import (
	"context"
	"database/sql"
	"encoding/json"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	queryUpdatePillar = sqlutil.Stmt{
		Name: "UpdatePillar",
		Query: `
SELECT
	code.update_pillar(
		i_value => :pillar,
		i_cid   => :cid,
		i_rev   => :rev,
		i_key   => code.make_pillar_key(
			i_cid      => :pillar_cid,
			i_subcid   => :pillar_subcid,
			i_shard_id => :pillar_shard_id,
			i_fqdn     => :pillar_fqdn
		)
	)`,
	}

	queryAddPillar = sqlutil.Stmt{
		Name: "AddPillar",
		Query: `
SELECT
	code.add_pillar(
		i_value => :pillar,
		i_cid   => :cid,
		i_rev   => :rev,
		i_key   => code.make_pillar_key(
			i_cid      => :pillar_cid,
			i_subcid   => :pillar_subcid,
			i_shard_id => :pillar_shard_id,
			i_fqdn     => :pillar_fqdn
		)
	)`,
	}

	queryAddTargetPillar = sqlutil.Stmt{
		Name: "AddTargetPillar",
		Query: `
INSERT INTO dbaas.target_pillar (
    target_id,
    cid,
    subcid,
    shard_id,
    fqdn,
    value
)
VALUES (
    :target_id,
    :pillar_cid,
    :pillar_subcid,
    :pillar_shard_id,
    :pillar_fqdn,
    :pillar
	)`,
	}

	queryGetClusterTypePillar = sqlutil.Stmt{
		Name: "ClusterTypePillar",
		Query: `
SELECT
    value
FROM
    dbaas.cluster_type_pillar
WHERE
    type = :cluster_type`,
	}

	queryGetHostPillar = sqlutil.Stmt{
		Name: "HostPillar",
		Query: `
SELECT
	value
FROM
	code.get_pillar_by_host(:fqdn, :target_id)
WHERE
	priority = 'fqdn'`,
	}
)

func (b *Backend) UpdateSubClusterPillar(ctx context.Context, cid, subcid string, revision int64, pillar json.RawMessage) error {
	return b.UpdatePillar(
		ctx,
		cid,
		revision,
		pillar,
		map[string]interface{}{
			"pillar_cid":      sql.NullString{},
			"pillar_subcid":   subcid,
			"pillar_shard_id": sql.NullString{},
			"pillar_fqdn":     sql.NullString{},
		},
	)
}

func (b *Backend) UpdateClusterPillar(ctx context.Context, cid string, revision int64, pillar json.RawMessage) error {
	return b.UpdatePillar(
		ctx,
		cid,
		revision,
		pillar,
		map[string]interface{}{
			"pillar_cid":      cid,
			"pillar_subcid":   sql.NullString{},
			"pillar_shard_id": sql.NullString{},
			"pillar_fqdn":     sql.NullString{},
		},
	)
}

func (b *Backend) UpdatePillar(ctx context.Context, cid string, revision int64, pillar json.RawMessage, args map[string]interface{}) error {
	args["cid"] = cid
	args["rev"] = revision
	args["pillar"] = string(pillar)

	_, err := sqlutil.QueryTx(
		ctx,
		queryUpdatePillar,
		args,
		sqlutil.NopParser,
		b.logger,
	)

	return err
}

func (b *Backend) AddSubClusterPillar(ctx context.Context, cid, subcid string, revision int64, pillar json.RawMessage) error {
	return b.AddPillar(
		ctx,
		cid,
		revision,
		pillar,
		map[string]interface{}{
			"pillar_cid":      sql.NullString{},
			"pillar_subcid":   subcid,
			"pillar_shard_id": sql.NullString{},
			"pillar_fqdn":     sql.NullString{},
		},
	)
}

func (b *Backend) AddClusterPillar(ctx context.Context, cid string, revision int64, pillar json.RawMessage) error {
	return b.AddPillar(
		ctx,
		cid,
		revision,
		pillar,
		map[string]interface{}{
			"pillar_cid":      cid,
			"pillar_subcid":   sql.NullString{},
			"pillar_shard_id": sql.NullString{},
			"pillar_fqdn":     sql.NullString{},
		},
	)
}

func (b *Backend) AddShardPillar(ctx context.Context, cid, shardid string, revision int64, pillar json.RawMessage) error {
	return b.AddPillar(
		ctx,
		cid,
		revision,
		pillar,
		map[string]interface{}{
			"pillar_cid":      sql.NullString{},
			"pillar_subcid":   sql.NullString{},
			"pillar_shard_id": shardid,
			"pillar_fqdn":     sql.NullString{},
		},
	)
}

func (b *Backend) AddHostPillar(ctx context.Context, cid string, fqdn string, revision int64, marshaller pillars.Marshaler) error {
	pillar, err := marshaller.MarshalPillar()
	if err != nil {
		return xerrors.Errorf("failed to marshal host pillar to raw form: %w", err)
	}
	return b.AddPillar(
		ctx,
		cid,
		revision,
		pillar,
		map[string]interface{}{
			"pillar_cid":      sql.NullString{},
			"pillar_subcid":   sql.NullString{},
			"pillar_shard_id": sql.NullString{},
			"pillar_fqdn":     fqdn,
		},
	)
}

func (b *Backend) UpdateHostPillar(ctx context.Context, cid string, fqdn string, revision int64, marshaller pillars.Marshaler) error {
	pillar, err := marshaller.MarshalPillar()
	if err != nil {
		return xerrors.Errorf("failed to marshal host pillar to raw form: %w", err)
	}
	return b.UpdatePillar(
		ctx,
		cid,
		revision,
		pillar,
		map[string]interface{}{
			"pillar_cid":      sql.NullString{},
			"pillar_subcid":   sql.NullString{},
			"pillar_shard_id": sql.NullString{},
			"pillar_fqdn":     fqdn,
		},
	)
}

func (b *Backend) AddPillar(ctx context.Context, cid string, revision int64, pillar json.RawMessage, args map[string]interface{}) error {
	args["cid"] = cid
	args["rev"] = revision
	args["pillar"] = string(pillar)
	_, err := sqlutil.QueryTx(
		ctx,
		queryAddPillar,
		args,
		sqlutil.NopParser,
		b.logger,
	)

	return err
}

func (b *Backend) AddTargetPillar(ctx context.Context, targetID string, pillar json.RawMessage, args map[string]interface{}) error {
	args["target_id"] = targetID
	args["pillar"] = string(pillar)
	fields := []string{"pillar_cid", "pillar_subcid", "pillar_shard_id", "pillar_fqdn"}
	for _, f := range fields {
		if _, ok := args[f]; !ok {
			args[f] = nil
		}
	}
	_, err := sqlutil.QueryTx(
		ctx,
		queryAddTargetPillar,
		args,
		sqlutil.NopParser,
		b.logger,
	)

	return err
}

func (b *Backend) ClusterTypePillar(ctx context.Context, typ clusters.Type, marshaller pillars.Marshaler) error {
	var pillar json.RawMessage
	parser := func(rows *sqlx.Rows) error {
		if err := rows.Scan(&pillar); err != nil {
			return err
		}
		return nil
	}
	_, err := sqlutil.QueryContext(
		ctx,
		b.cluster.AliveChooser(),
		queryGetClusterTypePillar,
		map[string]interface{}{
			"cluster_type": typ.Stringified(),
		},
		parser,
		b.logger)
	if err != nil {
		return err
	}
	return marshaller.UnmarshalPillar(pillar)
}

func (b *Backend) HostPillar(ctx context.Context, fqdn string, marshaller pillars.Marshaler) error {
	var pillar json.RawMessage
	parser := func(rows *sqlx.Rows) error {
		if err := rows.Scan(&pillar); err != nil {
			return err
		}
		return nil
	}
	_, err := sqlutil.QueryContext(
		ctx,
		b.cluster.AliveChooser(),
		queryGetHostPillar,
		map[string]interface{}{
			"fqdn":      fqdn,
			"target_id": sql.NullString{},
		},
		parser,
		b.logger)
	if err != nil {
		return err
	}
	return marshaller.UnmarshalPillar(pillar)
}

func (b *Backend) UpdateShardPillar(ctx context.Context, cid, shardid string, revision int64, pillar json.RawMessage) error {
	return b.UpdatePillar(
		ctx,
		cid,
		revision,
		pillar,
		map[string]interface{}{
			"pillar_cid":      sql.NullString{},
			"pillar_subcid":   sql.NullString{},
			"pillar_shard_id": shardid,
			"pillar_fqdn":     sql.NullString{},
		},
	)
}
