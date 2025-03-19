package operation

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/operation"
	"a.yandex-team.ru/cloud/marketplace/pkg/tracing"
	"a.yandex-team.ru/library/go/core/log"
)

type OperationServiceClient interface {
	Get(ctx context.Context, opID string) (*operation.Operation, error)
	WaitOperation(ctx context.Context, operationID string) (*operation.Operation, error)
}

func (c *Client) Get(ctx context.Context, operationID string) (*operation.Operation, error) {
	span, spanCtx := tracing.Start(ctx, "operation:Get")
	defer span.Finish()

	s := operation.NewOperationServiceClient(c.connection)
	req := &operation.GetOperationRequest{OperationId: operationID}

	op, err := s.Get(spanCtx, req)
	if err := c.mapError(spanCtx, err); err != nil {
		return nil, err
	}
	return op, nil
}

func (c *Client) WaitOperation(ctx context.Context, operationID string) (*operation.Operation, error) {
	ctx, cancel := context.WithTimeout(ctx, c.OpTimeout)
	defer cancel()

	scopedLogger := log.With(c.logger, log.String("operation_id", operationID))
	for {
		scopedLogger.Info("polling operation ...")

		op, err := c.Get(ctx, operationID)
		if err != nil {
			scopedLogger.Debug("operation polling failed")
			return nil, fmt.Errorf("failed to poll operation: %v", err)
		}

		if op.Done {
			scopedLogger.Info("operation completed")

			switch v := op.Result.(type) {
			case *operation.Operation_Error:
				return op, fmt.Errorf(v.Error.String())
			case *operation.Operation_Response:
				return op, nil
			}
			return op, nil
		}
		select {
		case <-ctx.Done():
			return nil, fmt.Errorf("timeout waiting for operation: %v", ctx.Err())
		case <-time.After(c.OpPollInterval):
		}
	}
}
