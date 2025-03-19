package grpc

import (
	"context"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	mlockapi "a.yandex-team.ru/cloud/mdb/mlock/api"
)

func (bl *GRPCLocker) ReleaseLock(ctx context.Context, lockID string) error {
	_, err := bl.mlock.ReleaseLock(ctx, &mlockapi.ReleaseLockRequest{
		Id: lockID,
	})
	if err != nil {
		statusCode, ok := status.FromError(err)
		if !ok {
			return semerr.WrapWithInternal(err, "cannot release lock")
		}
		if statusCode.Code() == codes.NotFound {
			return semerr.WrapWithNotFound(err, "lock not found")
		}
		return err
	}
	return nil
}
