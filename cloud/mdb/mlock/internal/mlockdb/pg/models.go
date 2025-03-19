package pg

import (
	"time"

	"github.com/jackc/pgtype"

	"a.yandex-team.ru/cloud/mdb/mlock/internal/models"
)

// Lock is in-db representation of Lock model
type Lock struct {
	ID       string           `db:"lock_ext_id"`
	Holder   string           `db:"holder"`
	Reason   string           `db:"reason"`
	Objects  pgtype.TextArray `db:"objects"`
	CreateTS time.Time        `db:"create_ts"`
}

// ToInternal casts in-db Lock model into internal one
func (lock *Lock) ToInternal() models.Lock {
	out := models.Lock{
		ID:       models.LockID(lock.ID),
		Holder:   models.LockHolder(lock.Holder),
		Reason:   models.LockReason(lock.Reason),
		CreateTS: lock.CreateTS,
	}

	for _, object := range lock.Objects.Elements {
		out.Objects = append(out.Objects, models.LockObject(object.String))
	}

	return out
}

// Conflict is in-db representation of Lock conflict
type Conflict struct {
	Object string   `json:"object"`
	Ids    []string `json:"ids"`
}

// LockStatus is in-db representation of Lock status
type LockStatus struct {
	ID        string           `db:"lock_ext_id"`
	Holder    string           `db:"holder"`
	Reason    string           `db:"reason"`
	Objects   pgtype.TextArray `db:"objects"`
	CreateTS  time.Time        `db:"create_ts"`
	Conflicts pgtype.JSON      `db:"conflicts"`
	Acquired  bool             `db:"acquired"`
}

// ToInternal casts in-db LockStatus model into internal one
func (status *LockStatus) ToInternal() (models.LockStatus, error) {
	out := models.LockStatus{
		ID:       models.LockID(status.ID),
		Holder:   models.LockHolder(status.Holder),
		Reason:   models.LockReason(status.Reason),
		CreateTS: status.CreateTS,
		Acquired: status.Acquired,
	}

	for _, object := range status.Objects.Elements {
		out.Objects = append(out.Objects, models.LockObject(object.String))
	}

	var conflicts []Conflict
	err := status.Conflicts.AssignTo(&conflicts)
	if err != nil {
		return models.LockStatus{}, err
	}

	for _, conflict := range conflicts {
		var ids []models.LockID

		for _, lockID := range conflict.Ids {
			ids = append(ids, models.LockID(lockID))
		}

		out.Conflicts = append(out.Conflicts, models.LockConflict{
			Object: models.LockObject(conflict.Object),
			Ids:    ids,
		})
	}

	return out, nil
}
