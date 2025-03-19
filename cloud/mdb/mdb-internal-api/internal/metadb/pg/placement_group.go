package pg

import (
	"context"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/sqlerrors"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
)

var (
	queryAddDiskPlacementGroup = sqlutil.Stmt{
		Name: "AddDiskPlacementGroup",
		Query: `
SELECT code.create_disk_placement_group(
	i_cid              => :cid,
	i_local_id         => :local_id,
	i_rev              => :rev
) pg_id`,
	}

	queryAddDisk = sqlutil.Stmt{
		Name: "AddDisk",
		Query: `
SELECT code.create_disk(
	i_cid              => :cid,
	i_local_id         => :local_id,
	i_fqdn             => :fqdn,
	i_mount_point      => :mount_point,
	i_rev              => :rev
) d_id`,
	}

	queryAddPlacementGroup = sqlutil.Stmt{
		Name: "AddPlacementGroup",
		Query: `
SELECT code.create_placement_group(
	i_cid              => :cid,
	i_rev              => :rev,
	i_fqdn			   => :fqdn,
	i_local_id         => :local_id
) pg_id`,
	}
)

func (b *Backend) AddDiskPlacementGroup(ctx context.Context, args models.AddDiskPlacementGroupArgs) (int64, error) {
	var res struct {
		PlacementGroupID int64 `db:"pg_id"`
	}
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&res)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		queryAddDiskPlacementGroup,
		map[string]interface{}{
			"cid":      args.ClusterID,
			"local_id": args.LocalID,
			"rev":      args.Revision,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return res.PlacementGroupID, err
	}
	if count == 0 {
		return res.PlacementGroupID, sqlerrors.ErrNotFound
	}

	return res.PlacementGroupID, nil
}

func (b *Backend) AddDisk(ctx context.Context, args models.AddDiskArgs) (int64, error) {
	var res struct {
		DiskID int64 `db:"d_id"`
	}
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&res)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		queryAddDisk,
		map[string]interface{}{
			"cid":         args.ClusterID,
			"local_id":    args.LocalID,
			"fqdn":        args.FQDN,
			"mount_point": args.MountPoint,
			"rev":         args.Revision,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return res.DiskID, err
	}
	if count == 0 {
		return res.DiskID, sqlerrors.ErrNotFound
	}

	return res.DiskID, nil
}

func (b *Backend) AddPlacementGroup(ctx context.Context, args models.AddPlacementGroupArgs) (int64, error) {
	var res struct {
		PlacementGroupID int64 `db:"pg_id"`
	}
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&res)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		queryAddPlacementGroup,
		map[string]interface{}{
			"cid":      args.ClusterID,
			"rev":      args.Revision,
			"fqdn":     args.FQDN,
			"local_id": args.LocalID,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return res.PlacementGroupID, err
	}
	if count == 0 {
		return res.PlacementGroupID, sqlerrors.ErrNotFound
	}

	return res.PlacementGroupID, nil
}
