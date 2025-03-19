package license

import (
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/db/ydb"
	"a.yandex-team.ru/cloud/marketplace/license_server/pkg/utils"
	ydb_pkg "a.yandex-team.ru/cloud/marketplace/pkg/ydb"
)

type LockState string

const (
	LockedLockState   LockState = "locked"
	UnlockedLockState LockState = "unlocked"
	DeletedLockState  LockState = "deleted"
)

type Lock struct {
	ID             string    `json:"id"`
	InstanceID     string    `json:"license_instance_id"`
	ResourceLockID string    `json:"resource_lock_id"`
	StartTime      time.Time `json:"start_time"`
	EndTime        time.Time `json:"end_time"`
	CreatedAt      time.Time `json:"created_at"`
	UpdatedAt      time.Time `json:"updated_at"`
	State          LockState `json:"state"`
}

func NewLockFromYDB(ll *ydb.LicenseLock) (*Lock, error) {
	if ll == nil {
		return nil, nil
	}

	out := &Lock{
		ID:             ll.ID,
		InstanceID:     ll.InstanceID,
		ResourceLockID: ll.ResourceLockID,
		StartTime:      time.Time(ll.StartTime),
		EndTime:        time.Time(ll.EndTime),
		CreatedAt:      time.Time(ll.CreatedAt),
		UpdatedAt:      time.Time(ll.UpdatedAt),
	}

	switch ll.State {
	case "locked":
		out.State = LockedLockState
	case "unlocked":
		out.State = UnlockedLockState
	case "deleted":
		out.State = DeletedLockState
	}

	return out, nil
}

func (ll *Lock) Proto() (*license.Lock, error) {
	if ll == nil {
		return nil, fmt.Errorf("license template parameter is nil")
	}

	out := &license.Lock{
		Id:             ll.ID,
		InstanceId:     ll.InstanceID,
		ResourceLockId: ll.ResourceLockID,
		StartTime:      utils.GetTimestamppbFromTime(ll.StartTime),
		EndTime:        utils.GetTimestamppbFromTime(ll.EndTime),
		CreatedAt:      utils.GetTimestamppbFromTime(ll.CreatedAt),
		UpdatedAt:      utils.GetTimestamppbFromTime(ll.UpdatedAt),
	}

	out.StartTime.Nanos = 0

	switch ll.State {
	case LockedLockState:
		out.State = license.Lock_LOCKED
	case UnlockedLockState:
		out.State = license.Lock_UNLOCKED
	case DeletedLockState:
		out.State = license.Lock_DELETED
	default:
		out.State = license.Lock_STATE_UNSPECIFIED
	}

	return out, nil
}

func (ll *Lock) YDB() (*ydb.LicenseLock, error) {
	if ll == nil {
		return nil, nil
	}

	out := &ydb.LicenseLock{
		ID:             ll.ID,
		InstanceID:     ll.InstanceID,
		ResourceLockID: ll.ResourceLockID,
		StartTime:      ydb_pkg.UInt64Ts(ll.StartTime),
		EndTime:        ydb_pkg.UInt64Ts(ll.EndTime),
		CreatedAt:      ydb_pkg.UInt64Ts(ll.CreatedAt),
		UpdatedAt:      ydb_pkg.UInt64Ts(ll.UpdatedAt),
		State:          string(ll.State),
	}

	return out, nil
}
