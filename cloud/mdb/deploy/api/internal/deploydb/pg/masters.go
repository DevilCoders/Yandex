package pg

import (
	"context"
	"database/sql"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/deploy/api/internal/deploydb"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
)

var (
	queryCreateMaster = sqlutil.Stmt{
		Name: "CreateMaster",
		Query: "SELECT * FROM code.create_master(" +
			"i_fqdn => :fqdn, " +
			"i_deploy_group => :group, " +
			"i_is_open => :is_open, " +
			"i_description => :description)",
	}
	queryUpsertMaster = sqlutil.Stmt{
		Name: "UpsertMaster",
		Query: "SELECT * FROM code.upsert_master(" +
			"i_fqdn => :fqdn, " +
			"i_deploy_group => :group, " +
			"i_is_open => :is_open, " +
			"i_description => :description)",
	}
	querySelectMaster = sqlutil.Stmt{
		Name:  "SelectMaster",
		Query: "SELECT * FROM code.get_master(i_fqdn => :fqdn)",
	}
	querySelectMasters = sqlutil.Stmt{
		Name: "SelectMasters",
		Query: "SELECT * FROM code.get_masters(" +
			"i_limit => :limit, " +
			"i_last_master_id => :last_master_id)",
	}
	queryUpdateMasterCheck = sqlutil.Stmt{
		Name: "UpdateMasterAlive",
		Query: "SELECT * FROM code.update_master_check(" +
			"i_master_fqdn => :master_fqdn, " +
			"i_checker_fqdn => :checker_fqdn, " +
			"i_is_alive => :is_alive)",
	}
)

func (b *backend) CreateMaster(ctx context.Context, fqdn, group string, isOpen bool, desc string) (models.Master, error) {
	var master masterModel
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&master)
	}

	args := map[string]interface{}{
		"fqdn":        fqdn,
		"group":       group,
		"is_open":     isOpen,
		"description": desc,
	}
	if _, err := sqlutil.QueryContext(
		ctx,
		b.cluster.PrimaryChooser(),
		queryCreateMaster,
		args,
		parser,
		b.logger,
	); err != nil {
		return models.Master{}, errorToSemErr(err)
	}

	return masterFromDB(master), nil
}

func (b *backend) UpsertMaster(ctx context.Context, fqdn string, attrs deploydb.UpsertMasterAttrs) (models.Master, error) {
	var master masterModel
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&master)
	}

	if _, err := sqlutil.QueryContext(
		ctx,
		b.cluster.PrimaryChooser(),
		queryUpsertMaster,
		map[string]interface{}{
			"fqdn":        fqdn,
			"group":       sql.NullString(attrs.Group),
			"is_open":     sql.NullBool(attrs.IsOpen),
			"description": sql.NullString(attrs.Description),
		},
		parser,
		b.logger,
	); err != nil {
		return models.Master{}, errorToSemErr(err)
	}

	return masterFromDB(master), nil
}

func (b *backend) Master(ctx context.Context, fqdn string) (models.Master, error) {
	return b.queryMaster(ctx, b.cluster.AliveChooser(), querySelectMaster, map[string]interface{}{"fqdn": fqdn})
}

func (b *backend) Masters(ctx context.Context, limit int64, lastMasterID optional.Int64) ([]models.Master, error) {
	var masters []models.Master
	parser := func(rows *sqlx.Rows) error {
		var master masterModel
		if err := rows.StructScan(&master); err != nil {
			return err
		}

		masters = append(masters, masterFromDB(master))
		return nil
	}

	args := map[string]interface{}{
		"limit": limit,
	}
	if lastMasterID.Valid {
		args["last_master_id"] = lastMasterID.Must()
	} else {
		args["last_master_id"] = nil
	}

	if _, err := sqlutil.QueryContext(
		ctx,
		b.cluster.AliveChooser(),
		querySelectMasters,
		args,
		parser,
		b.logger,
	); err != nil {
		return nil, errorToSemErr(err)
	}

	return masters, nil
}

func (b *backend) UpdateMasterCheck(ctx context.Context, masterFQDN, checkerFQDN string, alive bool) (models.Master, error) {
	return b.queryMaster(
		ctx,
		b.cluster.PrimaryChooser(),
		queryUpdateMasterCheck,
		map[string]interface{}{
			"master_fqdn":  masterFQDN,
			"checker_fqdn": checkerFQDN,
			"is_alive":     alive,
		},
	)
}

func (b *backend) queryMaster(ctx context.Context, chooser sqlutil.NodeChooser, stmt sqlutil.Stmt, args map[string]interface{}) (models.Master, error) {
	var master masterModel
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&master)
	}

	count, err := sqlutil.QueryContext(
		ctx,
		chooser,
		stmt,
		args,
		parser,
		b.logger,
	)
	if err != nil {
		return models.Master{}, errorToSemErr(err)
	}
	if count == 0 {
		return models.Master{}, semerr.NotFound("master not found")
	}

	return masterFromDB(master), nil
}
