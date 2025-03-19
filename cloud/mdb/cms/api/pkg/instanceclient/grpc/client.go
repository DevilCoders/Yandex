package grpc

import (
	"context"
	"fmt"

	"google.golang.org/grpc/credentials"

	api "a.yandex-team.ru/cloud/mdb/cms/api/grpcapi/v1"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/instanceclient"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Config struct {
	Host      string                `json:"host" yaml:"host"`
	Transport grpcutil.ClientConfig `json:"transport" yaml:"transport"`
}

func DefaultConfig() Config {
	return Config{
		Transport: grpcutil.DefaultClientConfig(),
	}
}

type InstanceClient struct {
	isc api.InstanceServiceClient
	osc api.InstanceOperationServiceClient
	dsc api.DutyServiceClient
}

func (c *InstanceClient) List(ctx context.Context) (*api.ListInstanceOperationsResponse, error) {
	result, err := c.osc.List(ctx, &api.ListInstanceOperationsRequest{})
	return result, err
}

var _ instanceclient.InstanceClient = &InstanceClient{}

func (c *InstanceClient) CreateMoveInstanceOperation(ctx context.Context, instanceID string, comment string, force bool) (*api.InstanceOperation, error) {
	return c.isc.MoveInstance(ctx, &api.MoveInstanceRequest{
		InstanceId: instanceID,
		Comment:    comment,
		Force:      force,
	})
}

func (c *InstanceClient) CreateWhipPrimaryOperation(ctx context.Context, instanceID string, comment string) (*api.InstanceOperation, error) {
	return c.isc.WhipPrimary(ctx, &api.WhipPrimaryRequest{
		InstanceId: instanceID,
		Comment:    comment,
	})
}

func (c *InstanceClient) GetInstanceOperation(ctx context.Context, operationID string) (*api.InstanceOperation, error) {
	return c.osc.Get(ctx, &api.GetInstanceOperationRequest{Id: operationID})
}

func (c *InstanceClient) AlarmOperations(ctx context.Context) (*api.AlarmResponse, error) {
	return c.dsc.Alarm(ctx, &api.AlarmRequest{})
}

func (c *InstanceClient) ChangeOperationStatus(
	ctx context.Context, operationID string, status models.InstanceOperationStatus,
) error {
	req := &api.ChangeStatusRequest{
		Id: operationID,
	}

	switch status {
	case models.InstanceOperationStatusInProgress:
		req.Status = api.ChangeStatusRequest_PROCESSING
	case models.InstanceOperationStatusOK:
		req.Status = api.ChangeStatusRequest_OK
	default:
		return xerrors.Errorf("unsupported new status: %q", status)
	}

	_, err := c.dsc.ChangeStatus(ctx, req)

	return err
}

func (c *InstanceClient) ResolveInstancesByDom0(ctx context.Context, dom0s []string) (*api.InstanceListResponce, error) {
	return c.dsc.ResolveInstancesByDom0(ctx, &api.ResolveInstancesByDom0Request{Dom0: dom0s})
}

func (c *InstanceClient) Dom0Instances(ctx context.Context, dom0 string) (*api.Dom0InstancesResponse, error) {
	return c.dsc.Dom0Instances(ctx, &api.Dom0InstancesRequest{Dom0: dom0})
}

func NewFromConfig(ctx context.Context, cfg Config, appName string, l log.Logger, creds credentials.PerRPCCredentials) (instanceclient.InstanceClient, error) {
	conn, err := grpcutil.NewConn(ctx, cfg.Host, appName, cfg.Transport, l, grpcutil.WithClientCredentials(creds))
	if err != nil {
		return nil, fmt.Errorf("failed to initialize grpc connection %w", err)
	}

	return &InstanceClient{
		isc: api.NewInstanceServiceClient(conn),
		osc: api.NewInstanceOperationServiceClient(conn),
		dsc: api.NewDutyServiceClient(conn),
	}, nil
}
