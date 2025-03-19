package grpc

import (
	"context"

	mlockapi "a.yandex-team.ru/cloud/mdb/mlock/api"
	"a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient"
)

func (bl *GRPCLocker) GetLockStatus(ctx context.Context, id string) (mlockclient.LockStatus, error) {
	response, err := bl.mlock.GetLockStatus(ctx, &mlockapi.GetLockStatusRequest{
		Id: id,
	})
	if err != nil {
		return mlockclient.LockStatus{}, err
	}
	return mlockclient.LockStatus{
		ID:        response.Id,
		Acquired:  response.Acquired,
		Conflicts: conflictsFromConflictSlice(response.Conflicts),
		Holder:    response.Holder,
		Reason:    response.Reason,
	}, nil
}
