package snapshot

import (
	"context"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/accounting"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/common"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage"
)

////////////////////////////////////////////////////////////////////////////////

type snapshotTarget struct {
	uniqueID         string
	snapshotID       string
	storage          storage.Storage
	ignoreZeroChunks bool
	rewriteChunks    bool
	useS3            bool
}

func (t *snapshotTarget) Write(
	ctx context.Context,
	chunk common.Chunk,
) error {

	if t.ignoreZeroChunks && chunk.Zero {
		return nil
	}

	var err error
	if t.rewriteChunks {
		_, err = t.storage.RewriteChunk(
			ctx,
			t.uniqueID,
			t.snapshotID,
			chunk,
			t.useS3,
		)
	} else {
		_, err = t.storage.WriteChunk(
			ctx,
			t.uniqueID,
			t.snapshotID,
			chunk,
			t.useS3,
		)
	}
	if err != nil {
		return err
	}

	accounting.OnSnapshotWrite(t.snapshotID, len(chunk.Data))
	return nil
}

func (t *snapshotTarget) Close(ctx context.Context) {
}

////////////////////////////////////////////////////////////////////////////////

func CreateSnapshotTarget(
	uniqueID string,
	snapshotID string,
	storage storage.Storage,
	ignoreZeroChunks bool,
	rewriteChunks bool,
	useS3 bool,
) common.Target {

	return &snapshotTarget{
		uniqueID:         uniqueID,
		snapshotID:       snapshotID,
		storage:          storage,
		ignoreZeroChunks: ignoreZeroChunks,
		rewriteChunks:    rewriteChunks,
		useS3:            useS3,
	}
}
