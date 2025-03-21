package client

import (
	"context"
	"fmt"
	"time"

	grpc_retry "github.com/grpc-ecosystem/go-grpc-middleware/retry"
	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	"a.yandex-team.ru/cloud/disk_manager/pkg/client/config"
)

////////////////////////////////////////////////////////////////////////////////

func NewClient(
	ctx context.Context,
	config *config.Config,
	options ...grpc.DialOption,
) (Client, error) {

	perRetryTimeout, err := time.ParseDuration(config.GetPerRetryTimeout())
	if err != nil {
		return nil, fmt.Errorf("failed to parse PerRetryTimeout from config: %w", err)
	}

	backoffTimeout, err := time.ParseDuration(config.GetBackoffTimeout())
	if err != nil {
		return nil, fmt.Errorf("failed to parse BackoffTimeout from config: %w", err)
	}

	operationPollPeriod, err := time.ParseDuration(config.GetOperationPollPeriod())
	if err != nil {
		return nil, fmt.Errorf("failed to parse OperationPollPeriod from config: %w", err)
	}

	interceptor := grpc_retry.UnaryClientInterceptor(
		grpc_retry.WithMax(uint(config.GetMaxRetryAttempts())),
		grpc_retry.WithPerRetryTimeout(perRetryTimeout),
		grpc_retry.WithBackoff(func(attempt uint) time.Duration { return backoffTimeout }),
	)
	options = append(options, grpc.WithChainUnaryInterceptor(interceptor))

	conn, err := grpc.DialContext(ctx, config.GetEndpoint(), options...)
	if err != nil {
		return nil, err
	}

	return &client{
		operationPollPeriod: operationPollPeriod,
		conn:                conn,

		diskServiceClient:           disk_manager.NewDiskServiceClient(conn),
		imageServiceClient:          disk_manager.NewImageServiceClient(conn),
		operationServiceClient:      disk_manager.NewOperationServiceClient(conn),
		placementGroupServiceClient: disk_manager.NewPlacementGroupServiceClient(conn),
		snapshotServiceClient:       disk_manager.NewSnapshotServiceClient(conn),
		filesystemServiceClient:     disk_manager.NewFilesystemServiceClient(conn),
	}, nil
}

////////////////////////////////////////////////////////////////////////////////

type client struct {
	operationPollPeriod time.Duration
	conn                *grpc.ClientConn

	diskServiceClient           disk_manager.DiskServiceClient
	imageServiceClient          disk_manager.ImageServiceClient
	operationServiceClient      disk_manager.OperationServiceClient
	placementGroupServiceClient disk_manager.PlacementGroupServiceClient
	snapshotServiceClient       disk_manager.SnapshotServiceClient
	filesystemServiceClient     disk_manager.FilesystemServiceClient
}

////////////////////////////////////////////////////////////////////////////////

func (c *client) CreateDisk(
	ctx context.Context,
	req *disk_manager.CreateDiskRequest,
) (*operation.Operation, error) {

	return c.diskServiceClient.Create(ctx, req)
}

func (c *client) DeleteDisk(
	ctx context.Context,
	req *disk_manager.DeleteDiskRequest,
) (*operation.Operation, error) {

	return c.diskServiceClient.Delete(ctx, req)
}

func (c *client) ResizeDisk(
	ctx context.Context,
	req *disk_manager.ResizeDiskRequest,
) (*operation.Operation, error) {

	return c.diskServiceClient.Resize(ctx, req)
}

func (c *client) AlterDisk(
	ctx context.Context,
	req *disk_manager.AlterDiskRequest,
) (*operation.Operation, error) {

	return c.diskServiceClient.Alter(ctx, req)
}

func (c *client) AssignDisk(
	ctx context.Context,
	req *disk_manager.AssignDiskRequest,
) (*operation.Operation, error) {

	return c.diskServiceClient.Assign(ctx, req)
}

func (c *client) UnassignDisk(
	ctx context.Context,
	req *disk_manager.UnassignDiskRequest,
) (*operation.Operation, error) {

	return c.diskServiceClient.Unassign(ctx, req)
}

func (c *client) DescribeDiskModel(
	ctx context.Context,
	req *disk_manager.DescribeDiskModelRequest,
) (*disk_manager.DiskModel, error) {

	return c.diskServiceClient.DescribeModel(ctx, req)
}

func (c *client) StatDisk(
	ctx context.Context,
	req *disk_manager.StatDiskRequest,
) (*disk_manager.DiskStats, error) {

	return c.diskServiceClient.Stat(ctx, req)
}

func (c *client) MigrateDisk(
	ctx context.Context,
	req *disk_manager.MigrateDiskRequest,
) (*operation.Operation, error) {

	return c.diskServiceClient.Migrate(ctx, req)
}

func (c *client) SendMigrationSignal(
	ctx context.Context,
	req *disk_manager.SendMigrationSignalRequest,
) error {

	_, err := c.diskServiceClient.SendMigrationSignal(ctx, req)
	return err
}

func (c *client) CreateImage(
	ctx context.Context,
	req *disk_manager.CreateImageRequest,
) (*operation.Operation, error) {

	return c.imageServiceClient.Create(ctx, req)
}

func (c *client) UpdateImage(
	ctx context.Context,
	req *disk_manager.UpdateImageRequest,
) (*operation.Operation, error) {

	return c.imageServiceClient.Update(ctx, req)
}

func (c *client) DeleteImage(
	ctx context.Context,
	req *disk_manager.DeleteImageRequest,
) (*operation.Operation, error) {

	return c.imageServiceClient.Delete(ctx, req)
}

func (c *client) GetOperation(
	ctx context.Context,
	req *disk_manager.GetOperationRequest,
) (*operation.Operation, error) {

	return c.operationServiceClient.Get(ctx, req)
}

func (c *client) CancelOperation(
	ctx context.Context,
	req *disk_manager.CancelOperationRequest,
) (*operation.Operation, error) {

	return c.operationServiceClient.Cancel(ctx, req)
}

func (c *client) WaitOperation(
	ctx context.Context,
	operationID string,
	callback func(context.Context, *operation.Operation) error,
) (*operation.Operation, error) {

	for {
		o, err := c.operationServiceClient.Get(ctx, &disk_manager.GetOperationRequest{
			OperationId: operationID,
		})
		if err != nil {
			return nil, err
		}

		if callback != nil {
			err = callback(ctx, o)
			if err != nil {
				return nil, err
			}
		}

		if o.Done {
			return o, nil
		}

		select {
		case <-ctx.Done():
			return nil, ctx.Err()
		case <-time.After(c.operationPollPeriod):
		}
	}
}

func (c *client) CreatePlacementGroup(
	ctx context.Context,
	req *disk_manager.CreatePlacementGroupRequest,
) (*operation.Operation, error) {

	return c.placementGroupServiceClient.Create(ctx, req)
}

func (c *client) DeletePlacementGroup(
	ctx context.Context,
	req *disk_manager.DeletePlacementGroupRequest,
) (*operation.Operation, error) {

	return c.placementGroupServiceClient.Delete(ctx, req)
}

func (c *client) AlterPlacementGroupMembership(
	ctx context.Context,
	req *disk_manager.AlterPlacementGroupMembershipRequest,
) (*operation.Operation, error) {

	return c.placementGroupServiceClient.Alter(ctx, req)
}

func (c *client) ListPlacementGroups(
	ctx context.Context,
	req *disk_manager.ListPlacementGroupsRequest,
) (*disk_manager.ListPlacementGroupsResponse, error) {

	return c.placementGroupServiceClient.List(ctx, req)
}

func (c *client) DescribePlacementGroup(
	ctx context.Context,
	req *disk_manager.DescribePlacementGroupRequest,
) (*disk_manager.PlacementGroup, error) {

	return c.placementGroupServiceClient.Describe(ctx, req)
}

func (c *client) CreateSnapshot(
	ctx context.Context,
	req *disk_manager.CreateSnapshotRequest,
) (*operation.Operation, error) {

	return c.snapshotServiceClient.Create(ctx, req)
}

func (c *client) DeleteSnapshot(
	ctx context.Context,
	req *disk_manager.DeleteSnapshotRequest,
) (*operation.Operation, error) {

	return c.snapshotServiceClient.Delete(ctx, req)
}

func (c *client) RestoreDiskFromSnapshot(
	ctx context.Context,
	req *disk_manager.RestoreDiskFromSnapshotRequest,
) (*operation.Operation, error) {

	return c.snapshotServiceClient.RestoreDisk(ctx, req)
}

func (c *client) Close() error {
	return c.conn.Close()
}

func (c *client) CreateFilesystem(
	ctx context.Context,
	req *disk_manager.CreateFilesystemRequest,
) (*operation.Operation, error) {

	return c.filesystemServiceClient.Create(ctx, req)
}

func (c *client) DeleteFilesystem(
	ctx context.Context,
	req *disk_manager.DeleteFilesystemRequest,
) (*operation.Operation, error) {

	return c.filesystemServiceClient.Delete(ctx, req)
}

func (c *client) ResizeFilesystem(
	ctx context.Context,
	req *disk_manager.ResizeFilesystemRequest,
) (*operation.Operation, error) {

	return c.filesystemServiceClient.Resize(ctx, req)
}

func (c *client) DescribeFilesystemModel(
	ctx context.Context,
	req *disk_manager.DescribeFilesystemModelRequest,
) (*disk_manager.FilesystemModel, error) {

	return c.filesystemServiceClient.DescribeModel(ctx, req)
}
