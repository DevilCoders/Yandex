package pg

import (
	"context"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/deploy/api/internal/deploydb"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/library/go/core/log"
)

const (
	commandColumns = "id, shipment_id, type, arguments, fqdn, " +
		"status, created_at, updated_at, timeout "
)

var (
	querySelectCommand = sqlutil.Stmt{
		Name:  "SelectCommand",
		Query: "SELECT " + commandColumns + " FROM code.get_command(i_command_id => :command_id)",
	}
	querySelectCommands = sqlutil.Stmt{
		Name: "SelectCommands",
		Query: "SELECT " + commandColumns + " FROM code.get_commands(" +
			"i_shipment_id => :shipment_id, " +
			"i_fqdn => :fqdn, " +
			"i_status => :status, " +
			"i_limit => :limit, " +
			"i_last_command_id => :last_command_id, " +
			"i_ascending => :ascending)",
	}
	querySelectCommandsForDispatch = sqlutil.Stmt{
		Name:  "SelectCommandsForDispatch",
		Query: "SELECT " + commandColumns + " FROM code.get_commands_for_dispatch(i_fqdn => :fqdn, i_limit => :limit, i_throttling_count => :throttling_count)",
	}
	queryCommandDispatchFailed = sqlutil.Stmt{
		Name:  "CommandDispatchFailed",
		Query: "SELECT * FROM code.command_dispatch_failed(i_command_id => :command_id)",
	}
)

func (b *backend) Command(ctx context.Context, id models.CommandID) (models.Command, error) {
	var command commandModel
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&command)
	}

	count, err := sqlutil.QueryContext(
		ctx,
		b.cluster.AliveChooser(),
		querySelectCommand,
		map[string]interface{}{"command_id": id},
		parser,
		b.logger,
	)
	if err != nil {
		return models.Command{}, errorToSemErr(err)
	}
	if count == 0 {
		return models.Command{}, semerr.NotFound("command not found")
	}

	return commandFromDB(command), nil
}

func (b *backend) Commands(ctx context.Context, attrs deploydb.SelectCommandsAttrs, limit int64, lastCommandID optional.Int64) ([]models.Command, error) {
	var cmds []models.Command
	parser := func(rows *sqlx.Rows) error {
		var cmd commandModel
		if err := rows.StructScan(&cmd); err != nil {
			return err
		}

		cmds = append(cmds, commandFromDB(cmd))
		return nil
	}

	args := map[string]interface{}{
		"limit":     limit,
		"ascending": sortOrderToDB(attrs.SortOrder),
	}
	if lastCommandID.Valid {
		args["last_command_id"] = lastCommandID.Must()
	} else {
		args["last_command_id"] = nil
	}

	if attrs.ShipmentID.Valid {
		args["shipment_id"] = attrs.ShipmentID.Int64
	} else {
		args["shipment_id"] = nil
	}

	if attrs.FQDN.Valid {
		args["fqdn"] = attrs.FQDN.String
	} else {
		args["fqdn"] = nil
	}

	if attrs.Status.Valid {
		args["status"] = attrs.Status.String
	} else {
		args["status"] = nil
	}

	_, err := sqlutil.QueryContext(
		ctx,
		b.cluster.AliveChooser(),
		querySelectCommands,
		args,
		parser,
		b.logger,
	)
	return cmds, errorToSemErr(err)
}

func (b *backend) CommandsForDispatch(ctx context.Context, fqdn string, limit int64, throttlingCount int32) (deploydb.CommandDispatcher, error) {
	dispatcher := commandDispatcher{lg: b.logger, b: b}
	parser := func(rows *sqlx.Rows) error {
		var cmd commandModel
		if err := rows.StructScan(&cmd); err != nil {
			return err
		}

		dispatcher.cmds = append(dispatcher.cmds, commandFromDB(cmd))
		return nil
	}

	binding, err := sqlutil.Begin(ctx, b.cluster, sqlutil.Primary, nil)
	if err != nil {
		return nil, err
	}
	dispatcher.binding = binding

	count, err := sqlutil.QueryTxBinding(
		ctx,
		dispatcher.binding,
		querySelectCommandsForDispatch,
		map[string]interface{}{
			"fqdn":             fqdn,
			"limit":            limit,
			"throttling_count": throttlingCount,
		},
		parser,
		b.logger,
	)
	if err != nil {
		_ = dispatcher.binding.Rollback(ctx)
		return nil, errorToSemErr(err)
	}
	if count == 0 {
		_ = dispatcher.binding.Rollback(ctx)
		return nil, semerr.NotFound("no commands found")
	}

	return &dispatcher, nil
}

type commandDispatcher struct {
	binding sqlutil.TxBinding
	lg      log.Logger
	fqdn    string
	cmds    []models.Command
	b       *backend
}

var _ deploydb.CommandDispatcher = &commandDispatcher{}

func (cd *commandDispatcher) Close(ctx context.Context) error {
	return cd.binding.Commit(ctx)
}

func (cd *commandDispatcher) MasterFQDN() string {
	return cd.fqdn
}

func (cd *commandDispatcher) Commands() []models.Command {
	return cd.cmds
}

func (cd *commandDispatcher) Dispatched(ctx context.Context, cmdID models.CommandID, extJobID string) (models.Job, error) {
	var job jobModel
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&job)
	}

	if _, err := sqlutil.QueryTxBinding(
		ctx,
		cd.binding,
		queryCreateJob,
		map[string]interface{}{
			"ext_job_id": extJobID,
			"command_id": cmdID,
		},
		parser,
		cd.b.logger,
	); err != nil {
		return models.Job{}, errorToSemErr(err)
	}

	return jobFromDB(job), nil
}

func (cd *commandDispatcher) DispatchFailed(ctx context.Context, id models.CommandID) (models.Command, error) {
	var command commandModel
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&command)
	}

	count, err := sqlutil.QueryTxBinding(
		ctx,
		cd.binding,
		queryCommandDispatchFailed,
		map[string]interface{}{"command_id": id},
		parser,
		cd.b.logger,
	)
	if err != nil {
		return models.Command{}, errorToSemErr(err)
	}
	if count == 0 {
		return models.Command{}, semerr.NotFound("command not found")
	}

	return commandFromDB(command), nil
}
