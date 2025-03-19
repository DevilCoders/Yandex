package server

import (
	"context"
	"errors"

	"github.com/golang/protobuf/ptypes"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	mlock "a.yandex-team.ru/cloud/mdb/mlock/api"
	"a.yandex-team.ru/cloud/mdb/mlock/internal/mlockdb"
	"a.yandex-team.ru/cloud/mdb/mlock/internal/models"
	"a.yandex-team.ru/library/go/core/log"
)

const (
	defaultLimit = 100
)

// MlockService is a grpc implementation of mlock
type MlockService struct {
	Auth    *Auth
	MlockDB mlockdb.MlockDB
	Logger  log.Logger
}

var _ mlock.LockServiceServer = &MlockService{}

func (service *MlockService) checkAuth(ctx context.Context) error {
	if service.Auth == nil {
		return nil
	}

	token, err := ParseGRPCAuthToken(ctx)

	if err != nil {
		return semerr.Authenticationf("%v", err)
	}

	_, err = service.Auth.Client.Auth(ctx, token, service.Auth.Permission, service.Auth.FolderID)

	if err != nil {
		return semerr.Authenticationf("%v", err)
	}

	return nil
}

// ListLocks returns existing locks without status
func (service *MlockService) ListLocks(ctx context.Context, req *mlock.ListLocksRequest) (*mlock.ListLocksResponse, error) {
	if err := service.checkAuth(ctx); err != nil {
		return nil, err
	}

	holder := req.GetHolder()

	limit := req.GetLimit()

	if limit == 0 {
		limit = defaultLimit
	} else if limit < 0 {
		return nil, semerr.InvalidInput("negative limit")
	}

	offset := req.GetOffset()

	if offset < 0 {
		return nil, semerr.InvalidInput("negative offset")
	}

	locks, hasMore, err := service.MlockDB.GetLocks(ctx, models.LockHolder(holder), limit, offset)

	if err != nil {
		return nil, semerr.Unavailablef("%v", err)
	}

	resp := mlock.ListLocksResponse{}

	if hasMore {
		resp.NextOffset = offset + limit
	}

	for _, lock := range locks {
		ts, err := ptypes.TimestampProto(lock.CreateTS)

		if err != nil {
			return nil, semerr.Unavailablef("%v", err)
		}

		out := mlock.Lock{
			Id:       string(lock.ID),
			Holder:   string(lock.Holder),
			Reason:   string(lock.Reason),
			CreateTs: ts,
		}

		for _, object := range lock.Objects {
			out.Objects = append(out.Objects, string(object))
		}

		resp.Locks = append(resp.Locks, &out)
	}

	return &resp, nil
}

// GetLockStatus returns single lock status
func (service *MlockService) GetLockStatus(ctx context.Context, req *mlock.GetLockStatusRequest) (*mlock.LockStatus, error) {
	if err := service.checkAuth(ctx); err != nil {
		return nil, err
	}

	id := req.GetId()

	if id == "" {
		return nil, semerr.InvalidInput("empty lock id")
	}

	status, err := service.MlockDB.GetLockStatus(ctx, models.LockID(id))

	if err != nil {
		if errors.Is(err, mlockdb.ErrNotFound) {
			return nil, semerr.NotFoundf("%v", err)
		}
		return nil, semerr.Unavailablef("%v", err)
	}

	ts, err := ptypes.TimestampProto(status.CreateTS)

	if err != nil {
		return nil, semerr.Unavailablef("%v", err)
	}

	resp := mlock.LockStatus{
		Id:       string(status.ID),
		Holder:   string(status.Holder),
		Reason:   string(status.Reason),
		CreateTs: ts,
		Acquired: status.Acquired,
	}

	for _, object := range status.Objects {
		resp.Objects = append(resp.Objects, string(object))
	}

	for _, conflict := range status.Conflicts {
		var ids []string

		for _, lockID := range conflict.Ids {
			ids = append(ids, string(lockID))
		}
		resp.Conflicts = append(resp.Conflicts, &mlock.Conflict{
			Object: string(conflict.Object),
			Ids:    ids,
		})
	}

	return &resp, nil
}

// CreateLock creates lock (idempotent method)
func (service *MlockService) CreateLock(ctx context.Context, req *mlock.CreateLockRequest) (*mlock.CreateLockResponse, error) {
	if err := service.checkAuth(ctx); err != nil {
		return nil, err
	}

	id := req.GetId()

	if id == "" {
		return nil, semerr.InvalidInput("empty lock id")
	}

	holder := req.GetHolder()

	if holder == "" {
		return nil, semerr.InvalidInput("empty lock holder")
	}

	reason := req.GetReason()

	if reason == "" {
		return nil, semerr.InvalidInput("empty lock reason")
	}

	objects := req.GetObjects()

	if len(objects) == 0 {
		return nil, semerr.InvalidInput("empty object list")
	}

	lock := models.Lock{
		ID:     models.LockID(id),
		Holder: models.LockHolder(holder),
		Reason: models.LockReason(reason),
	}

	objectsSet := make(map[string]bool, len(objects))

	for _, object := range objects {
		if _, ok := objectsSet[object]; ok {
			return nil, semerr.InvalidInputf("duplicate object %s", object)
		}
		objectsSet[object] = true
		lock.Objects = append(lock.Objects, models.LockObject(object))
	}

	err := service.MlockDB.CreateLock(ctx, lock)

	if err != nil {
		if errors.Is(err, mlockdb.ErrConflict) {
			return nil, semerr.InvalidInputf("%v", err)
		}
		return nil, semerr.Unavailablef("%v", err)
	}

	resp := mlock.CreateLockResponse{}

	return &resp, nil
}

// ReleaseLock releases lock (non-idempotent method)
func (service *MlockService) ReleaseLock(ctx context.Context, req *mlock.ReleaseLockRequest) (*mlock.ReleaseLockResponse, error) {
	if err := service.checkAuth(ctx); err != nil {
		return nil, err
	}

	id := req.GetId()

	if id == "" {
		return nil, semerr.InvalidInput("empty lock id")
	}

	err := service.MlockDB.ReleaseLock(ctx, models.LockID(id))

	if err != nil {
		if errors.Is(err, mlockdb.ErrNotFound) {
			return nil, semerr.NotFoundf("%v", err)
		}
		return nil, semerr.Unavailablef("%v", err)
	}

	resp := mlock.ReleaseLockResponse{}

	return &resp, nil
}
