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
	queryCreateMinion = sqlutil.Stmt{
		Name: "CreateMinion",
		Query: "SELECT * FROM code.create_minion(" +
			"i_fqdn => :fqdn, " +
			"i_deploy_group => :group, " +
			"i_auto_reassign => :auto_reassign)",
	}
	queryUpsertMinion = sqlutil.Stmt{
		Name: "UpsertMinion",
		Query: "SELECT * FROM code.upsert_minion(" +
			"i_fqdn => :fqdn, " +
			"i_deploy_group => :group, " +
			"i_auto_reassign => :auto_reassign," +
			"i_allow_recreate => :allow_recreate, " +
			"i_master => :master)",
	}
	querySelectMinion = sqlutil.Stmt{
		Name:  "SelectMinion",
		Query: "SELECT * FROM code.get_minion(i_fqdn => :fqdn)",
	}
	querySelectMinions = sqlutil.Stmt{
		Name: "SelectMinions",
		Query: "SELECT * FROM code.get_minions(" +
			"i_limit => :limit, " +
			"i_last_minion_id => :last_minion_id)",
	}
	querySelectMinionsByMaster = sqlutil.Stmt{
		Name: "SelectMinionsByMaster",
		Query: "SELECT * FROM code.get_minions_by_master(" +
			"i_master_fqdn => :master_fqdn, " +
			"i_limit => :limit, " +
			"i_last_minion_id => :last_minion_id)",
	}
	queryRegisterMinion = sqlutil.Stmt{
		Name:  "RegisterMinion",
		Query: "SELECT * FROM code.register_minion(i_fqdn => :fqdn, i_pub_key => :pub_key)",
	}
	queryUnregisterMinion = sqlutil.Stmt{
		Name:  "UnregisterMinion",
		Query: "SELECT * FROM code.unregister_minion(i_fqdn => :fqdn)",
	}
	queryDeleteMinion = sqlutil.Stmt{
		Name:  "DeleteMinion",
		Query: "SELECT * FROM code.delete_minion(i_fqdn => :fqdn)",
	}
	queryFailoverMinions = sqlutil.Stmt{
		Name:  "FailoverMinions",
		Query: "SELECT * FROM code.failover_minions(i_limit => :limit)",
	}
)

func (b *backend) CreateMinion(ctx context.Context, fqdn, group string, autoReassign bool) (models.Minion, error) {
	var minion minionModel
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&minion)
	}

	args := map[string]interface{}{
		"fqdn":          fqdn,
		"group":         group,
		"auto_reassign": autoReassign,
	}
	if _, err := sqlutil.QueryContext(
		ctx,
		b.cluster.PrimaryChooser(),
		queryCreateMinion,
		args,
		parser,
		b.logger,
	); err != nil {
		return models.Minion{}, errorToSemErr(err)
	}

	return minionFromDB(minion), nil
}

func (b *backend) UpsertMinion(ctx context.Context, fqdn string, attrs deploydb.UpsertMinionAttrs, allowRecreate bool) (models.Minion, error) {
	var minion minionModel
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&minion)
	}

	if _, err := sqlutil.QueryContext(
		ctx,
		b.cluster.PrimaryChooser(),
		queryUpsertMinion,
		map[string]interface{}{
			"fqdn":           fqdn,
			"group":          sql.NullString(attrs.Group),
			"auto_reassign":  sql.NullBool(attrs.AutoReassign),
			"allow_recreate": allowRecreate,
			"master":         sql.NullString(attrs.Master),
		},
		parser,
		b.logger,
	); err != nil {
		return models.Minion{}, errorToSemErr(err)
	}

	return minionFromDB(minion), nil
}

func (b *backend) Minion(ctx context.Context, fqdn string) (models.Minion, error) {
	var minion minionModel
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&minion)
	}

	count, err := sqlutil.QueryContext(
		ctx,
		b.cluster.AliveChooser(),
		querySelectMinion,
		map[string]interface{}{"fqdn": fqdn},
		parser,
		b.logger,
	)
	if err != nil {
		return models.Minion{}, errorToSemErr(err)
	}
	if count == 0 {
		return models.Minion{}, semerr.NotFound("minion not found")
	}

	return minionFromDB(minion), nil
}

func (b *backend) Minions(ctx context.Context, limit int64, lastMinionID optional.Int64) ([]models.Minion, error) {
	args := map[string]interface{}{
		"limit": limit,
	}
	if lastMinionID.Valid {
		args["last_minion_id"] = lastMinionID.Must()
	} else {
		args["last_minion_id"] = nil
	}

	return b.queryMinionsList(
		ctx,
		b.cluster.AliveChooser(),
		querySelectMinions,
		args,
	)
}

func (b *backend) FailoverMinions(ctx context.Context, limit int64) ([]models.Minion, error) {
	return b.queryMinionsList(
		ctx,
		b.cluster.PrimaryChooser(),
		queryFailoverMinions,
		map[string]interface{}{
			"limit": limit,
		},
	)
}

func (b *backend) queryMinionsList(ctx context.Context, chooser sqlutil.NodeChooser, stmt sqlutil.Stmt, args map[string]interface{}) ([]models.Minion, error) {
	var minions []models.Minion
	parser := func(rows *sqlx.Rows) error {
		var minion minionModel
		if err := rows.StructScan(&minion); err != nil {
			return err
		}

		minions = append(minions, minionFromDB(minion))
		return nil
	}

	if _, err := sqlutil.QueryContext(
		ctx,
		chooser,
		stmt,
		args,
		parser,
		b.logger,
	); err != nil {
		return nil, errorToSemErr(err)
	}

	return minions, nil
}

func (b *backend) MinionsByMaster(
	ctx context.Context,
	fqdn string,
	limit int64,
	lastMinionID optional.Int64,
) ([]models.Minion, error) {
	var minions []models.Minion
	parser := func(rows *sqlx.Rows) error {
		var minion minionModel
		if err := rows.StructScan(&minion); err != nil {
			return err
		}

		minions = append(minions, minionFromDB(minion))
		return nil
	}

	args := map[string]interface{}{
		"master_fqdn": fqdn,
		"limit":       limit,
	}
	if lastMinionID.Valid {
		args["last_minion_id"] = lastMinionID.Must()
	} else {
		args["last_minion_id"] = nil
	}

	if _, err := sqlutil.QueryContext(
		ctx,
		b.cluster.AliveChooser(),
		querySelectMinionsByMaster,
		args,
		parser,
		b.logger,
	); err != nil {
		return nil, errorToSemErr(err)
	}

	return minions, nil
}

func (b *backend) RegisterMinion(ctx context.Context, fqdn string, pubKey string) (models.Minion, error) {
	var minion minionModel
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&minion)
	}

	if _, err := sqlutil.QueryContext(
		ctx,
		b.cluster.PrimaryChooser(),
		queryRegisterMinion,
		map[string]interface{}{
			"fqdn":    fqdn,
			"pub_key": pubKey,
		},
		parser,
		b.logger,
	); err != nil {
		return models.Minion{}, errorToSemErr(err)
	}

	return minionFromDB(minion), nil
}

func (b *backend) UnregisterMinion(ctx context.Context, fqdn string) (models.Minion, error) {
	var minion minionModel
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&minion)
	}

	if _, err := sqlutil.QueryContext(
		ctx,
		b.cluster.PrimaryChooser(),
		queryUnregisterMinion,
		map[string]interface{}{
			"fqdn": fqdn,
		},
		parser,
		b.logger,
	); err != nil {
		return models.Minion{}, errorToSemErr(err)
	}

	return minionFromDB(minion), nil
}

func (b *backend) DeleteMinion(ctx context.Context, fqdn string) error {
	// TODO: WTF is this code?
	node := b.cluster.Primary()
	if node == nil {
		return semerr.Unavailable("unavailable")
	}

	args := map[string]interface{}{
		"fqdn": fqdn,
	}
	count, err := sqlutil.QueryContext(
		ctx,
		b.cluster.PrimaryChooser(),
		queryDeleteMinion,
		args,
		sqlutil.NopParser,
		b.logger,
	)
	if err != nil {
		return errorToSemErr(err)
	}
	if count == 0 {
		return semerr.NotFound("minion not found")
	}

	return nil
}
