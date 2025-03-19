package mlock

import (
	"context"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/lockcluster"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/tracing/tags"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (l *Locker) ReleaseCluster(ctx context.Context, state *lockcluster.State) error {
	lockID := state.LockID
	if lockID == "" {
		return nil
	}
	span, ctx := opentracing.StartSpanFromContext(ctx, "ReleaseCluster",
		tags.LockID.Tag(lockID),
	)
	defer span.Finish()

	if err := l.client.ReleaseLock(ctx, lockID); err != nil {
		if !semerr.IsNotFound(err) {
			return xerrors.Errorf("failed to release lock %q: %w", lockID, err)
		}
	}
	return nil
}
