package pg

import (
	"context"

	"github.com/jackc/pgconn"
	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	db_models "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/pg/internal/models"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	queryCreateNetwork = sqlutil.Stmt{
		Name: "CreateNetwork",
		// language=PostgreSQL
		Query: `
INSERT INTO vpc.networks (project_id, cloud_provider, region_id, name, description, ipv4_cidr_block, external_resources)
VALUES (:project_id, :cloud_provider, :region_id, :name, :description, :ipv4_cidr_block, :external_resources)
RETURNING network_id`}

	queryFinishNetworkCreating = sqlutil.Stmt{
		Name: "FinishNetworkCreating",
		// language=PostgreSQL
		Query: `
UPDATE vpc.networks
SET
    status = :status,
    external_resources = :external_resources,
    ipv6_cidr_block = :ipv6_cidr_block
WHERE network_id = :id
RETURNING network_id`}

	queryMarkNetworkDeleting = sqlutil.Stmt{
		Name: "MarkNetworkDeleting",
		// language=PostgreSQL
		Query: `
UPDATE vpc.networks
SET
    status = :status,
    status_reason = :status_reason
WHERE network_id = :id
RETURNING network_id`}

	queryDeleteNetwork = sqlutil.Stmt{
		Name: "DeleteNetwork",
		// language=PostgreSQL
		Query: `
DELETE FROM vpc.networks
WHERE network_id = :id
RETURNING network_id`}

	queryNetworkByID = sqlutil.NewStmt(
		"NetworkByID",
		// language=PostgreSQL
		`SELECT * from vpc.networks where network_id = :id`,
		db_models.Network{},
	)

	queryNetworksByProjectID = sqlutil.NewStmt(
		"NetworksByProjectID",
		// language=PostgreSQL
		`SELECT * from vpc.networks where project_id = :id`,
		db_models.Network{},
	)
	queryImportedNetworks = sqlutil.NewStmt(
		"ImportedNetworks",
		// language=PostgreSQL
		`SELECT * from vpc.networks
WHERE
      project_id = :id
  AND cloud_provider = :provider
  AND region_id = :region
  AND status != 'DELETING'`,
		db_models.Network{},
	)
)

// CreateNetwork must run inside a transaction
func (d *DB) CreateNetwork(
	ctx context.Context,
	projectID string,
	provider models.Provider,
	region string,
	name string,
	description string,
	ipv4CidrBlock string,
	externalResources models.ExternalResources,
) (string, error) {
	var netID string
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&netID)
	}

	dbRes, err := db_models.ExternalResourcesToDB(externalResources)
	if err != nil {
		return "", xerrors.Errorf("external resources to db: %w", err)
	}

	_, err = sqlutil.QueryTx(
		ctx,
		queryCreateNetwork,
		map[string]interface{}{
			"project_id":      projectID,
			"cloud_provider":  provider,
			"region_id":       region,
			"name":            name,
			"description":     description,
			"ipv4_cidr_block": ipv4CidrBlock,

			"external_resources": string(dbRes),
		},
		parser,
		d.logger,
	)
	if err != nil {
		var pgError *pgconn.PgError
		if xerrors.As(err, &pgError) {
			switch pgError.Code {
			case "23505":
				return "", semerr.FailedPreconditionf("network %q already exists", name)
			}
		}
	}

	return netID, err
}

// FinishNetworkCreating always runs outside a transaction
func (d *DB) FinishNetworkCreating(ctx context.Context, networkID string, ipv6CidrBlock string, resources models.ExternalResources) error {
	dbRes, err := db_models.ExternalResourcesToDB(resources)
	if err != nil {
		return xerrors.Errorf("operation state to db: %w", err)
	}

	c, err := sqlutil.QueryNode(
		ctx,
		d.cluster.Primary(),
		queryFinishNetworkCreating,
		map[string]interface{}{
			"status":             models.NetworkStatusActive,
			"external_resources": string(dbRes),
			"id":                 networkID,
			"ipv6_cidr_block":    ipv6CidrBlock,
		},
		sqlutil.NopParser,
		d.logger,
	)
	if err != nil {
		return err
	}
	if c == 0 {
		return semerr.NotFound("network not found")
	}
	return nil
}

func (d *DB) NetworkByID(ctx context.Context, networkID string) (models.Network, error) {
	var dbNet db_models.Network
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&dbNet)
	}

	c, err := sqlutil.QueryContext(
		ctx,
		d.cluster.PrimaryChooser(),
		queryNetworkByID,
		map[string]interface{}{
			"id": networkID,
		},
		parser,
		d.logger,
	)
	if err != nil {
		return models.Network{}, err
	}
	if c == 0 {
		return models.Network{}, semerr.NotFound("network was not found")
	}
	return dbNet.ToInternal()
}

// MarkNetworkDeleting must run inside a transaction
func (d *DB) MarkNetworkDeleting(ctx context.Context, networkID string, reason string) error {
	c, err := sqlutil.QueryTx(
		ctx,
		queryMarkNetworkDeleting,
		map[string]interface{}{
			"status":        models.NetworkStatusDeleting,
			"status_reason": reason,
			"id":            networkID,
		},
		sqlutil.NopParser,
		d.logger,
	)
	if err != nil {
		return err
	}
	if c == 0 {
		return semerr.NotFound("network not found")
	}
	return nil
}

// DeleteNetwork always runs outside a transaction
func (d *DB) DeleteNetwork(ctx context.Context, networkID string) error {
	c, err := sqlutil.QueryNode(
		ctx,
		d.cluster.Primary(),
		queryDeleteNetwork,
		map[string]interface{}{
			"id": networkID,
		},
		sqlutil.NopParser,
		d.logger,
	)
	if err != nil {
		return err
	}
	if c == 0 {
		return semerr.NotFound("network not found")
	}
	return nil
}

func (d *DB) NetworksByProjectID(ctx context.Context, projectID string) ([]models.Network, error) {
	res := make([]models.Network, 0)
	parser := func(rows *sqlx.Rows) error {
		var dbNet db_models.Network
		err := rows.StructScan(&dbNet)
		if err != nil {
			return err
		}

		net, err := dbNet.ToInternal()
		if err != nil {
			return err
		}

		res = append(res, net)
		return nil
	}

	_, err := sqlutil.QueryContext(
		ctx,
		d.cluster.PrimaryChooser(),
		queryNetworksByProjectID,
		map[string]interface{}{
			"id": projectID,
		},
		parser,
		d.logger,
	)
	if err != nil {
		return nil, err
	}
	return res, nil
}

func (d *DB) ImportedNetworks(ctx context.Context, projectID string, region string, provider models.Provider) ([]models.Network, error) {
	res := make([]models.Network, 0)
	parser := func(rows *sqlx.Rows) error {
		var dbNet db_models.Network
		err := rows.StructScan(&dbNet)
		if err != nil {
			return err
		}

		net, err := dbNet.ToInternal()
		if err != nil {
			return err
		}

		if net.IsImported() {
			res = append(res, net)
		}
		return nil
	}

	_, err := sqlutil.QueryContext(
		ctx,
		d.cluster.PrimaryChooser(),
		queryImportedNetworks,
		map[string]interface{}{
			"id":       projectID,
			"region":   region,
			"provider": provider,
		},
		parser,
		d.logger,
	)
	if err != nil {
		return nil, err
	}
	return res, nil
}
