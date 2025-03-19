package pg

import (
	"context"
	"time"

	"github.com/jackc/pgtype"
	"github.com/jmoiron/sqlx"
	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/deploy/api/internal/deploydb"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/tracing"
	"a.yandex-team.ru/library/go/core/log"
)

const (
	shipmentColumns = "shipment_id, commands, fqdns, status, " +
		"parallel, stop_on_error_count, other_count, done_count, errors_count, total_count, " +
		"created_at, updated_at, timeout, tracing"
)

var (
	queryCreateShipment = sqlutil.Stmt{
		Name: "CreateShipment",
		Query: "SELECT " + shipmentColumns + " " +
			"FROM code.create_shipment(" +
			"i_commands => code.commands_def_from_json(:commands), " +
			"i_fqdns => :fqdns, " +
			"i_skip_fqdns => :skip_fqdns, " +
			"i_parallel => :parallel, " +
			"i_stop_on_error_count => :stop_on_error_count," +
			"i_timeout => :timeout," +
			"i_tracing => :tracing)",
	}
	querySelectShipment = sqlutil.Stmt{
		Name: "SelectShipment",
		Query: "SELECT " + shipmentColumns + " " +
			"FROM code.get_shipment(i_shipment_id => :shipment_id)",
	}
	querySelectShipments = sqlutil.Stmt{
		Name: "SelectShipments",
		Query: "SELECT * FROM code.get_shipments(" +
			"i_fqdn => :fqdn, " +
			"i_status => :status, " +
			"i_limit => :limit, " +
			"i_last_shipment_id => :last_shipment_id, " +
			"i_ascending => :ascending)",
	}
	queryTimeoutShipments = sqlutil.Stmt{
		Name:  "TimeoutShipments",
		Query: "SELECT * FROM code.timeout_shipments(i_limit => :limit)",
	}
)

func (b *backend) CreateShipment(
	ctx context.Context,
	fqdns []string,
	skipFQDNs []string,
	commands []models.CommandDef,
	parallel, stopOnErrorCount int64,
	timeout time.Duration,
	carrier opentracing.TextMapCarrier,
) (models.Shipment, error) {
	var shipment shipmentModel
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&shipment)
	}

	stmt := queryCreateShipment
	logErr := func(msg string, err error) error {
		b.logger.Error(
			msg,
			log.String("name", stmt.Name),
			log.String("query", stmt.Query),
			log.Error(err),
		)
		return err
	}

	pgFQDNs := &pgtype.TextArray{}
	if err := pgFQDNs.Set(fqdns); err != nil {
		return models.Shipment{}, logErr("error while converting fqdns for statement", err)
	}

	pgSkipFQDNs := &pgtype.TextArray{}
	if err := pgSkipFQDNs.Set(skipFQDNs); err != nil {
		return models.Shipment{}, logErr("error while converting skip fqdns for statement", err)
	}

	pgCommands, err := commandDefToDB(commands)
	if err != nil {
		return models.Shipment{}, err
	}
	pgTimeout := &pgtype.Interval{}
	if err = pgTimeout.Set(timeout); err != nil {
		return models.Shipment{}, logErr("error while converting timeout for statement", err)
	}

	pgTracing, err := tracing.MarshalTextMapCarrier(carrier)
	if err != nil {
		return models.Shipment{}, logErr("failed to marshal shipment tracing carrier", err)
	}

	args := map[string]interface{}{
		"commands":            pgCommands,
		"fqdns":               pgFQDNs,
		"skip_fqdns":          pgSkipFQDNs,
		"parallel":            parallel,
		"stop_on_error_count": stopOnErrorCount,
		"timeout":             pgTimeout,
		"tracing":             string(pgTracing),
	}
	if _, err := sqlutil.QueryContext(
		ctx,
		b.cluster.PrimaryChooser(),
		stmt,
		args,
		parser,
		b.logger,
	); err != nil {
		return models.Shipment{}, errorToSemErr(err)
	}

	return shipmentFromDB(shipment)
}

func (b *backend) shipmentByID(ctx context.Context, id models.ShipmentID, chooser sqlutil.NodeChooser) (models.Shipment, error) {
	var shipment shipmentModel
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&shipment)
	}

	count, err := sqlutil.QueryContext(
		ctx,
		chooser,
		querySelectShipment,
		map[string]interface{}{"shipment_id": id},
		parser,
		b.logger,
	)
	if err != nil {
		return models.Shipment{}, errorToSemErr(err)
	}
	if count == 0 {
		return models.Shipment{}, semerr.NotFoundf("shipment %d not found", id)
	}

	return shipmentFromDB(shipment)
}

func (b *backend) Shipment(ctx context.Context, id models.ShipmentID) (models.Shipment, error) {
	ship, err := b.shipmentByID(ctx, id, b.cluster.AliveChooser())
	if semerr.IsNotFound(err) {
		return b.shipmentByID(ctx, id, b.cluster.PrimaryChooser())
	}
	return ship, err
}

func (b *backend) Shipments(ctx context.Context, attrs deploydb.SelectShipmentsAttrs, limit int64, lastShipmentID optional.Int64) ([]models.Shipment, error) {
	args := map[string]interface{}{
		"limit":            limit,
		"last_shipment_id": lastShipmentID,
		"ascending":        sortOrderToDB(attrs.SortOrder),
	}

	if lastShipmentID.Valid {
		args["last_shipment_id"] = lastShipmentID.Must()
	} else {
		args["last_shipment_id"] = nil
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

	return b.queryShipmentsList(ctx, b.cluster.AliveChooser(), querySelectShipments, args)
}

func (b *backend) TimeoutShipments(ctx context.Context, limit int64) ([]models.Shipment, error) {
	return b.queryShipmentsList(ctx, b.cluster.PrimaryChooser(), queryTimeoutShipments, map[string]interface{}{"limit": limit})
}

func (b *backend) queryShipmentsList(ctx context.Context, chooser sqlutil.NodeChooser, stmt sqlutil.Stmt, args map[string]interface{}) ([]models.Shipment, error) {
	var shipments []models.Shipment
	parser := func(rows *sqlx.Rows) error {
		var shipment shipmentModel
		if err := rows.StructScan(&shipment); err != nil {
			return err
		}

		s, err := shipmentFromDB(shipment)
		if err != nil {
			return sqlutil.NewParsingErrorf("error while converting shipment %q: %s", shipment.ID, err)
		}

		shipments = append(shipments, s)
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

	return shipments, nil
}
