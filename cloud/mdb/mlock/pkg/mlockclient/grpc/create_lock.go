package grpc

import (
	"context"

	mlockapi "a.yandex-team.ru/cloud/mdb/mlock/api"
)

func (bl *GRPCLocker) CreateLock(ctx context.Context, id, holder string, fqdns []string, reason string) error {
	_, err := bl.mlock.CreateLock(ctx, &mlockapi.CreateLockRequest{
		Id:      id,
		Holder:  holder,
		Reason:  reason,
		Objects: fqdns,
	})
	return err
}
