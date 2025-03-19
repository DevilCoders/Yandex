package monitoring

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/monrun"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil/pgerrors"
	"a.yandex-team.ru/cloud/mdb/katan/internal/katandb"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (m *Monrun) CheckZombieRollouts(ctx context.Context, warn, crit time.Duration) monrun.Result {
	roll, err := m.kdb.OldestRunningRollout(ctx)
	if err != nil {
		if xerrors.Is(err, katandb.ErrNoDataFound) {
			return monrun.Result{}
		}
		if pgerrors.IsTemporary(err) {
			return monrun.Warnf("temporary DB error: %s", err)
		}
		return monrun.Critf("unexpected DB error: %s", err)
	}

	age := time.Since(roll.CreatedAt)
	if roll.LastUpdatedAt.Valid {
		age = time.Since(roll.LastUpdatedAt.Time)
	}

	message := fmt.Sprintf("Exist zombie rollout. rollout_id: %d. It's age: %s", roll.ID, age)
	if roll.LastUpdatedAt.Valid {
		message += fmt.Sprintf(" Last updated cluster %s at %s", roll.LastUpdatedAt.Time, roll.LastUpdatedAtCluster.String)
	}
	switch {
	case age > crit:
		return monrun.Result{Code: monrun.CRIT, Message: message}
	case age > warn:
		return monrun.Result{Code: monrun.WARN, Message: message}
	}
	return monrun.Result{}
}
