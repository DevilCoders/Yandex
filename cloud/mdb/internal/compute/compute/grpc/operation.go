package grpc

import (
	"context"
	"time"

	"google.golang.org/grpc/credentials"

	cloudCompute "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/compute/v1"
	"a.yandex-team.ru/cloud/mdb/internal/compute/compute"
	"a.yandex-team.ru/cloud/mdb/internal/compute/operations"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var _ compute.OperationService = &OperationServiceClient{}

type OperationServiceClient struct {
	operationAPI cloudCompute.OperationServiceClient
	backoff      *retry.BackOff
	l            log.Logger
}

func NewOperationServiceClient(
	ctx context.Context,
	target, userAgent string,
	cfg grpcutil.ClientConfig,
	creds credentials.PerRPCCredentials,
	opDoneBackoff retry.Config,
	l log.Logger,
) (*OperationServiceClient, error) {
	conn, err := grpcutil.NewConn(ctx, target, userAgent, cfg, l, grpcutil.WithClientCredentials(creds))
	if err != nil {
		return nil, xerrors.Errorf("connecting to operation API at %q: %w", target, err)
	}

	return &OperationServiceClient{
		operationAPI: cloudCompute.NewOperationServiceClient(conn),
		backoff:      retry.New(opDoneBackoff),
		l:            l,
	}, nil
}

func (o *OperationServiceClient) Get(ctx context.Context, operationID string) (operations.Operation, error) {
	cloudOp, err := o.operationAPI.Get(ctx, &cloudCompute.GetOperationRequest{
		OperationId: operationID,
	})
	if err != nil {
		return operations.Operation{}, xerrors.Errorf("get operation: %w", err)
	}
	return operations.FromGRPC(cloudOp)
}

func (o *OperationServiceClient) Cancel(ctx context.Context, operationID string) (operations.Operation, error) {
	cloudOp, err := o.operationAPI.Cancel(ctx, &cloudCompute.CancelOperationRequest{
		OperationId: operationID,
	})
	if err != nil {
		return operations.Operation{}, xerrors.Errorf("cancel operation: %w", err)
	}
	return operations.FromGRPC(cloudOp)
}

var notDone = xerrors.NewSentinel("not done yet")

func (o *OperationServiceClient) GetDone(ctx context.Context, operationID string) (operations.Operation, error) {
	var doneOp operations.Operation
	err := o.backoff.RetryNotify(ctx, func() error {
		op, err := o.Get(ctx, operationID)
		if err != nil {
			return retry.Permanent(err)
		}
		if op.Done {
			doneOp = op
			return nil
		}
		return notDone
	}, func(err error, duration time.Duration) {
		ctxlog.Debug(ctx, o.l, "waiting for operation", log.String("operation_id", operationID))
	})
	if err != nil {
		return operations.Operation{}, err
	}
	return doneOp, nil
}
