package vpcdb

import (
	"context"
	"io"

	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
)

//go:generate ../../../scripts/mockgen.sh VPCDB

// VPCDB is a database interface
type VPCDB interface {
	ready.Checker
	io.Closer
	sqlutil.TxBinder

	OperationToProcess(ctx context.Context) (models.Operation, error)
	InsertOperation(
		ctx context.Context,
		projectID string,
		description string,
		createdBy string,
		params models.OperationParams,
		action models.OperationAction,
		provider models.Provider,
		region string,
	) (operationID string, err error)
	UpdateOperationFields(ctx context.Context, op models.Operation) error
	OperationByID(ctx context.Context, operationID string) (models.Operation, error)

	CreateNetwork(
		ctx context.Context,
		projectID string,
		provider models.Provider,
		region string,
		name string,
		description string,
		ipv4CidrBlock string,
		externalResources models.ExternalResources,
	) (string, error)
	FinishNetworkCreating(ctx context.Context, networkID string, ipv6CidrBlock string, resources models.ExternalResources) error
	NetworkByID(ctx context.Context, networkID string) (models.Network, error)
	MarkNetworkDeleting(ctx context.Context, networkID string, reason string) error
	DeleteNetwork(ctx context.Context, networkID string) error
	NetworksByProjectID(ctx context.Context, projectID string) ([]models.Network, error)
	ImportedNetworks(ctx context.Context, projectID string, region string, provider models.Provider) ([]models.Network, error)

	CreateNetworkConnection(
		ctx context.Context,
		networkID string,
		projectID string,
		provider models.Provider,
		region string,
		description string,
		params models.NetworkConnectionParams,
	) (string, error)
	NetworkConnectionByID(ctx context.Context, networkConnectionID string) (models.NetworkConnection, error)
	NetworkConnectionsByProjectID(ctx context.Context, projectID string) ([]models.NetworkConnection, error)
	NetworkConnectionsByNetworkID(ctx context.Context, networkID string) ([]models.NetworkConnection, error)
	MarkNetworkConnectionDeleting(ctx context.Context, networkConnectionID string, reason string) error
	MarkNetworkConnectionPending(ctx context.Context, networkConnectionID string, reason string, params models.NetworkConnectionParams) error
	FinishNetworkConnectionCreating(ctx context.Context, networkConnectionID string) error
	DeleteNetworkConnection(ctx context.Context, networkConnectionID string) error
}
