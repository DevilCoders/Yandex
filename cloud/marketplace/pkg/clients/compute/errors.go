package compute

import (
	"context"

	"google.golang.org/grpc/status"

	"a.yandex-team.ru/library/go/core/log"

	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
)

func (c *Client) mapError(ctx context.Context, requestErr error) error {
	scopedLogger := ctxtools.Logger(ctx)

	grpcErr := status.Convert(requestErr)

	if grpcErr == nil {
		return nil
	}

	scopedLogger.Error("failed to retrieve data from compute service", log.Error(grpcErr.Err()))

	return grpcErr.Err()
}
