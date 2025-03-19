package rm

import (
	"context"

	"golang.org/x/xerrors"

	"google.golang.org/grpc/status"

	"a.yandex-team.ru/library/go/core/log"

	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
)

func (c *Client) mapError(ctx context.Context, requestErr error) error {
	scopedLogger := ctxtools.Logger(ctx)

	if grpcErr := status.Convert(requestErr); grpcErr != nil {
		scopedLogger.Error("failed to retrieve data from resource manager", log.Error(grpcErr.Err()))
		return grpcErr.Err()
	}

	if xerrors.Is(requestErr, context.Canceled) || xerrors.Is(requestErr, context.DeadlineExceeded) {
		scopedLogger.Error("resource manager context has been canceled", log.Error(requestErr))
	}

	scopedLogger.Debug("resource manager request completed", log.Error(requestErr))

	return requestErr
}
