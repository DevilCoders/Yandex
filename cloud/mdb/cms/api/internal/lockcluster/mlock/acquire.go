package mlock

import (
	"context"
	"fmt"
	"strings"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/lockcluster"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/tracing/tags"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (l *Locker) LockCluster(ctx context.Context, fqdn string, taskID string, holder lockcluster.Holder) (*lockcluster.State, error) {
	span, ctx := opentracing.StartSpanFromContext(ctx, "LockCluster",
		tags.InstanceFQDN.Tag(fqdn),
		tags.CmsOperationID.Tag(taskID),
	)
	defer span.Finish()
	nhood, err := l.otherLegs.FindInShardOrSubcidByFQDN(ctx, fqdn)
	if err != nil {
		if semerr.IsNotFound(err) {
			return nil, lockcluster.UnmanagedHost
		}
		return nil, xerrors.Errorf("could not find neighbours for %q: %w", fqdn, err)
	}

	lockID := genLockID(taskID, nhood.ID)
	res := &lockcluster.State{LockID: lockID}
	legsToLock := make([]string, len(nhood.Others)+1)
	for ind, leg := range nhood.Others {
		legsToLock[ind] = leg.FQDN
	}
	legsToLock[len(legsToLock)-1] = fqdn

	err = l.client.CreateLock(ctx, lockID, string(holder), legsToLock, taskID)
	if err != nil {
		return res, xerrors.Errorf("could not create lock %q: %w", lockID, err)
	}

	status, err := l.client.GetLockStatus(ctx, lockID)
	if err != nil {
		return res, xerrors.Errorf("could not get lock %q status: %w", lockID, err)
	}

	if !status.Acquired {
		newLockID, err := l.reuse(ctx, fqdn, status)
		if err != nil {
			if xerrors.Is(err, lockcluster.WrongHolder) || xerrors.Is(err, lockcluster.NotAcquired) {
				conflictFQDNs := make([]string, len(status.Conflicts))
				for index, cnfl := range status.Conflicts {
					conflictFQDNs[index] = fmt.Sprintf(
						"%s locked by %s",
						cnfl.Object,
						strings.Join(cnfl.LockIDs, ", "),
					)
				}
				return res, lockcluster.NewError(
					lockcluster.NotAcquiredConflicts,
					fmt.Sprintf(
						"could not acquire lock %q because of conflicts: %s",
						lockID,
						strings.Join(conflictFQDNs, ", "),
					),
				)
			}
			return res, xerrors.Errorf("can lock be reused: %w", err)
		}
		res.LockID = newLockID
		if err := l.client.ReleaseLock(ctx, lockID); err != nil {
			return res, xerrors.Errorf("release old lock: %w", err)
		}
	}

	res.IsTaken = true
	return res, nil
}
