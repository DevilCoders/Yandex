package pg

import (
	"context"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	db_models "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/pg/internal/models"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	queryCreateNetworkConnection = sqlutil.Stmt{
		Name: "CreateNetworkConnection",
		// language=PostgreSQL
		Query: `
INSERT INTO vpc.network_connections (network_id, project_id, cloud_provider, region_id, description, connection_params)
VALUES (:network_id, :project_id, :cloud_provider, :region_id, :description, :connection_params)
RETURNING network_connection_id`}

	queryMarkNetworkConnectionDeleting = sqlutil.Stmt{
		Name: "MarkNetworkConnectionDeleting",
		// language=PostgreSQL
		Query: `
UPDATE vpc.network_connections
SET
    status = :status,
    status_reason = :status_reason
WHERE network_connection_id = :id
RETURNING network_connection_id`}

	queryMarkNetworkConnectionPending = sqlutil.Stmt{
		Name: "MarkNetworkConnectionPending",
		// language=PostgreSQL
		Query: `
UPDATE vpc.network_connections
SET
    status = :status,
    status_reason = :status_reason,
    connection_params = :connection_params
WHERE network_connection_id = :id
RETURNING network_connection_id`}

	queryFinishNetworkConnectionCreating = sqlutil.Stmt{
		Name: "FinishNetworkConnectionCreating",
		// language=PostgreSQL
		Query: `
UPDATE vpc.network_connections
SET
    status = :status,
    status_reason = ''
WHERE network_connection_id = :id
RETURNING network_connection_id`}

	queryNetworkConnectionByID = sqlutil.NewStmt(
		"NetworkConnectionByID",
		// language=PostgreSQL
		`SELECT * from vpc.network_connections where network_connection_id = :id`,
		db_models.NetworkConnection{},
	)

	queryNetworkConnectionsByProjectID = sqlutil.NewStmt(
		"NetworkConnectionsByProjectID",
		// language=PostgreSQL
		`SELECT * from vpc.network_connections where project_id = :id`,
		db_models.NetworkConnection{},
	)

	queryNetworkConnectionsByNetworkID = sqlutil.NewStmt(
		"NetworkConnectionsByNetworkID",
		// language=PostgreSQL
		`SELECT * from vpc.network_connections where network_id = :id`,
		db_models.NetworkConnection{},
	)

	queryDeleteNetworkConnection = sqlutil.Stmt{
		Name: "DeleteNetworkConnection",
		// language=PostgreSQL
		Query: `
DELETE FROM vpc.network_connections
WHERE network_connection_id = :id
RETURNING network_connection_id`}
)

func (d *DB) CreateNetworkConnection(
	ctx context.Context,
	networkID string,
	projectID string,
	provider models.Provider,
	region string,
	description string,
	params models.NetworkConnectionParams,
) (string, error) {
	var ncID string
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&ncID)
	}

	dbParams, err := db_models.NetworkConnectionParamsToDB(params)
	if err != nil {
		return "", xerrors.Errorf("operation state to db: %w", err)
	}

	_, err = sqlutil.QueryTx(
		ctx,
		queryCreateNetworkConnection,
		map[string]interface{}{
			"network_id":        networkID,
			"project_id":        projectID,
			"cloud_provider":    provider,
			"region_id":         region,
			"description":       description,
			"connection_params": string(dbParams),
		},
		parser,
		d.logger,
	)

	return ncID, err
}

func (d *DB) NetworkConnectionByID(ctx context.Context, networkConnectionID string) (models.NetworkConnection, error) {
	var dbNC db_models.NetworkConnection
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&dbNC)
	}

	c, err := sqlutil.QueryContext(
		ctx,
		d.cluster.PrimaryChooser(),
		queryNetworkConnectionByID,
		map[string]interface{}{
			"id": networkConnectionID,
		},
		parser,
		d.logger,
	)
	if err != nil {
		return models.NetworkConnection{}, err
	}
	if c == 0 {
		return models.NetworkConnection{}, semerr.NotFound("network connection was not found")
	}
	return dbNC.ToInternal()
}

func (d *DB) NetworkConnectionsByProjectID(ctx context.Context, projectID string) ([]models.NetworkConnection, error) {
	return d.listNetworkConnections(ctx, queryNetworkConnectionsByProjectID, map[string]interface{}{"id": projectID})
}

func (d *DB) NetworkConnectionsByNetworkID(ctx context.Context, networkID string) ([]models.NetworkConnection, error) {
	return d.listNetworkConnections(ctx, queryNetworkConnectionsByNetworkID, map[string]interface{}{"id": networkID})
}

func (d *DB) listNetworkConnections(ctx context.Context, stmt sqlutil.Stmt, args map[string]interface{}) ([]models.NetworkConnection, error) {
	res := make([]models.NetworkConnection, 0)
	parser := func(rows *sqlx.Rows) error {
		var dbNC db_models.NetworkConnection
		err := rows.StructScan(&dbNC)
		if err != nil {
			return err
		}

		nc, err := dbNC.ToInternal()
		if err != nil {
			return err
		}

		res = append(res, nc)
		return nil
	}

	_, err := sqlutil.QueryContext(
		ctx,
		d.cluster.PrimaryChooser(),
		stmt,
		args,
		parser,
		d.logger,
	)
	if err != nil {
		return nil, err
	}
	return res, nil
}

func (d *DB) MarkNetworkConnectionDeleting(ctx context.Context, networkConnectionID string, reason string) error {
	c, err := sqlutil.QueryTx(
		ctx,
		queryMarkNetworkConnectionDeleting,
		map[string]interface{}{
			"status":        models.NetworkConnectionStatusDeleting,
			"status_reason": reason,
			"id":            networkConnectionID,
		},
		sqlutil.NopParser,
		d.logger,
	)
	if err != nil {
		return err
	}
	if c == 0 {
		return semerr.NotFound("network connection not found")
	}
	return nil
}

// MarkNetworkConnectionPending always runs outside a transaction
func (d *DB) MarkNetworkConnectionPending(ctx context.Context, networkConnectionID string, reason string, params models.NetworkConnectionParams) error {
	dbParams, err := db_models.NetworkConnectionParamsToDB(params)
	if err != nil {
		return xerrors.Errorf("network connection params to DB: %w", err)
	}

	c, err := sqlutil.QueryNode(
		ctx,
		d.cluster.Primary(),
		queryMarkNetworkConnectionPending,
		map[string]interface{}{
			"status":            models.NetworkConnectionStatusPending,
			"connection_params": string(dbParams),
			"id":                networkConnectionID,
			"status_reason":     reason,
		},
		sqlutil.NopParser,
		d.logger,
	)
	if err != nil {
		return err
	}
	if c == 0 {
		return semerr.NotFound("network connection not found")
	}
	return nil
}

// FinishNetworkConnectionCreating always runs outside a transaction
func (d *DB) FinishNetworkConnectionCreating(ctx context.Context, networkConnectionID string) error {
	c, err := sqlutil.QueryNode(
		ctx,
		d.cluster.Primary(),
		queryFinishNetworkConnectionCreating,
		map[string]interface{}{
			"status": models.NetworkConnectionStatusActive,
			"id":     networkConnectionID,
		},
		sqlutil.NopParser,
		d.logger,
	)
	if err != nil {
		return err
	}
	if c == 0 {
		return semerr.NotFound("network connection not found")
	}
	return nil
}

// DeleteNetworkConnection always runs outside a transaction
func (d *DB) DeleteNetworkConnection(ctx context.Context, networkConnectionID string) error {
	c, err := sqlutil.QueryNode(
		ctx,
		d.cluster.Primary(),
		queryDeleteNetworkConnection,
		map[string]interface{}{
			"id": networkConnectionID,
		},
		sqlutil.NopParser,
		d.logger,
	)
	if err != nil {
		return err
	}
	if c == 0 {
		return semerr.NotFound("network connection not found")
	}
	return nil
}
