package mlock

import (
	"context"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/lockcluster"
	"a.yandex-team.ru/cloud/mdb/internal/tracing/tags"
	"a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (l *Locker) reuse(ctx context.Context, fqdn string, status mlockclient.LockStatus) (string, error) {
	lockID := status.ID
	span, ctx := opentracing.StartSpanFromContext(ctx, "IsLockReusable",
		tags.InstanceFQDN.Tag(fqdn),
		tags.LockID.Tag(lockID),
	)
	defer span.Finish()

	switch lockcluster.Holder(status.Holder) {
	case lockcluster.InstanceCMS:
	default:
		return "", xerrors.Errorf("only instance cms can reuse locks, got %s: %w", status.Holder, lockcluster.WrongHolder)
	}

	if status.Holder == string(lockcluster.WalleCMS) {
		return "", xerrors.Errorf("wall-e cms can not reuse locks: %w", lockcluster.WrongHolder)
	}

	for _, conflict := range status.Conflicts {
		if conflict.Object != fqdn {
			continue
		}
		// we rely on FIFO order of locks
		for _, lock := range conflict.LockIDs {
			lockStatus, err := l.client.GetLockStatus(ctx, lock)
			if err != nil {
				return "", xerrors.Errorf("get lock %q: %w", lock, err)
			}
			if !lockStatus.Acquired {
				return "", xerrors.Errorf("%q: %w", lock, lockcluster.NotAcquired)
			}
			if lockStatus.Holder != string(lockcluster.WalleCMS) {
				return "", xerrors.Errorf("%q is acquired by %s: %w", lock, lockStatus.Holder, lockcluster.WrongHolder)
			}
			return lock, nil
		}
	}

	return "", xerrors.Errorf("can not find conflicted lock for fqdn %q and lock %q, probably it is a bug", fqdn, lockID)
}
