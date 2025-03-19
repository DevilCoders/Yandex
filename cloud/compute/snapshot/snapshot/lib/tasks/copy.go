package tasks

import (
	"crypto/md5"
	"fmt"
	"sort"
	"sync"
	"time"

	"a.yandex-team.ru/cloud/compute/go-common/util"

	"github.com/prometheus/client_golang/prometheus"
	"go.uber.org/zap"
	"golang.org/x/net/context"
	"golang.org/x/sync/errgroup"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/chunker"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/storage"
)

var _ Task = &ShallowCopyContext{}

const OperationTypeShallowCopy = "shallow-copy"

// ShallowCopyContext implemements the shallow copy task.
type ShallowCopyContext struct {
	request *common.CopyRequest
	st      storage.Storage
	contextHeartbeat

	group         *errgroup.Group
	groupMixinCtx context.Context

	mutex    sync.Mutex
	finished bool
	err      error

	createdAt  time.Time
	finishedAt *time.Time
}

// NewShallowCopyContext returns a ShallowCopyContext instance.
func NewShallowCopyContext(st storage.Storage, request *common.CopyRequest) *ShallowCopyContext {
	return &ShallowCopyContext{
		request: request,
		st:      st,
		contextHeartbeat: contextHeartbeat{
			timeoutMs:  request.Params.HeartbeatTimeoutMs,
			statusChan: make(chan struct{}),
		},
		createdAt: time.Now(),
	}
}

// GetID returns task ID.
func (scc *ShallowCopyContext) GetID() string {
	return scc.request.Params.TaskID
}

func (scc *ShallowCopyContext) Type() string {
	return OperationTypeShallowCopy
}

func (scc *ShallowCopyContext) GetOperationCloudID() string {
	return scc.request.OperationCloudID
}

func (scc *ShallowCopyContext) GetOperationProjectID() string {
	if scc.request.OperationProjectID != "" {
		return scc.request.OperationProjectID
	}
	return scc.request.TargetProjectID
}

func (scc *ShallowCopyContext) GetSnapshotID() string {
	return scc.request.ID
}

func (scc *ShallowCopyContext) Timer(ctx context.Context) *prometheus.Timer {
	return misc.EndShallowCopyTimer.Start()
}

func (scc *ShallowCopyContext) RequestHash() string {
	return fmt.Sprintf("%x", md5.Sum([]byte(fmt.Sprintf("%v", scc.request))))
}

func (scc *ShallowCopyContext) CheckHeartbeat(err error) error {
	return scc.checkHeartbeat(err)
}

func (scc *ShallowCopyContext) GetHeartbeat() time.Duration {
	return time.Duration(scc.request.Params.HeartbeatTimeoutMs) * time.Millisecond
}

func (scc *ShallowCopyContext) GetRequest() common.TaskRequest {
	return scc.request
}

// GetStatus returns task status and updates heartbeat.
func (scc *ShallowCopyContext) GetStatus() *common.TaskStatus {
	if scc.request.Params.HeartbeatEnabled {
		// To update heartbeat timer.
		select {
		case <-scc.statusChan: // pass
		default: // pass
		}
	}

	scc.mutex.Lock()
	defer scc.mutex.Unlock()

	return &common.TaskStatus{
		Finished:   scc.finished,
		Success:    scc.err == nil,
		Error:      scc.err,
		CreatedAt:  scc.createdAt,
		FinishedAt: scc.finishedAt,
	}
}

// Cancel cancels the task.
func (scc *ShallowCopyContext) Cancel(ctx context.Context) {
	lock := util.MakeLockGuard(&scc.mutex)
	lock.Lock()
	defer lock.UnlockIfLocked()

	if scc.cancel == nil {
		return
	}

	if !scc.finished {
		scc.finished = true
		scc.err = misc.ErrTaskCancelled
	}
	lock.Unlock()

	log.G(ctx).Debug("Cancel ShallowCopyContext context")
	scc.cancel()
	_ = scc.group.Wait()
}

// Begin checks source and creates the target snapshot.
func (scc *ShallowCopyContext) Begin(ctx context.Context) error {
	sh, err := scc.st.GetSnapshot(ctx, scc.request.ID)
	if err != nil {
		return err
	}

	if sh.State.Code != storage.StateReady {
		log.G(ctx).Error("ShallowCopy.Begin: invalid state", zap.String("state", sh.State.Code))
		return misc.ErrSnapshotNotReady
	}

	var ci common.CreationInfo
	ci.ID = scc.request.TargetID
	ci.ProjectID = scc.request.TargetProjectID
	ci.Name = scc.request.Name
	ci.ImageID = scc.request.ImageID
	ci.Description = sh.Description
	ci.Size = sh.Size

	_, err = scc.st.BeginSnapshot(ctx, &ci)
	return err
}

func (scc *ShallowCopyContext) InitTaskWorkContext(ctx context.Context) context.Context {
	var heartbeatCtx context.Context
	// Used to cancel tasks due to API call or missed heartbeats.
	heartbeatCtx, scc.cancel = context.WithCancel(ctx)

	if scc.request.Params.HeartbeatEnabled {
		scc.startHeartbeat(heartbeatCtx)
	}

	scc.group, scc.groupMixinCtx = errgroup.WithContext(context.Background())

	return heartbeatCtx
}

// End starts the task.
func (scc *ShallowCopyContext) DoWork(ctx context.Context) (resErr error) {
	defer func() {
		log.G(ctx).Debug("Canceling scc context due to end of ShallowCopyContext.DoWork")
		scc.cancel()
		resErr = scc.checkHeartbeat(resErr)
	}()

	// Used to coordinate worker group.
	groupCtx := misc.CompoundContext(ctx, scc.groupMixinCtx)

	scc.group.Go(func() error {
		return scc.copy(groupCtx)
	})

	err := scc.group.Wait()
	if scc.request.Params.HeartbeatEnabled {
		// If heartbeat elapsed, we will have wrong err.
		err = scc.checkHeartbeat(err)
	}

	if err != nil {
		logger := log.G(ctx)
		statusFailed := storage.StateFailed
		logger.Warn("Error while copy. Update snapshot status.", zap.Error(err),
			zap.String("status", statusFailed))
		err = scc.st.UpdateSnapshotStatus(ctx, scc.request.TargetID, storage.StatusUpdate{
			State: statusFailed, Desc: err.Error()})
		if err != nil {
			logger.Error("Error set status to snapshot.", zap.Error(err))
		}
	}

	scc.finish(ctx, err)
	return err
}

func (scc *ShallowCopyContext) copy(ctx context.Context) (err error) {
	// By locking, we ensure that no chunks in source map
	// will be removed until we reference them.
	ctx, holder, err := scc.st.LockSnapshotShared(ctx, scc.request.ID, "ShallowCopyContext-fast-snapshot-copy-source")
	if err != nil {
		return err
	}
	defer func() {
		holder.Close(ctx)
		log.G(ctx).Debug("ShallowCopy.End: shared locks released")
	}()

	ctx, holder2, err := scc.st.LockSnapshot(ctx, scc.request.TargetID, "ShallowCopyContext-fast-snapshot-copy-target")
	defer func() {
		holder2.Close(ctx)
		log.G(ctx).Debug("ShallowCopy.End: target lock released")
	}()

	log.G(ctx).Debug("ShallowCopy.End: shared locks acquired")

	sh, err := scc.st.GetSnapshotFull(ctx, scc.request.ID)
	if err != nil {
		return err
	}

	// Check again to avoid races.
	if sh.State.Code != storage.StateReady {
		log.G(ctx).Error("ShallowCopy.End: invalid state", zap.String("state", sh.State.Code))
		return misc.ErrSnapshotNotReady
	}

	chunks, err := scc.st.GetChunksFromCache(ctx, scc.request.ID, false)
	if err != nil {
		return err
	}

	queue := make([]storage.ChunkRef, 0, chunks.Len())
	_ = chunks.Foreach(func(offset int64, chunk *storage.LibChunk) error {
		if !chunk.Zero {
			// Skip zero chunks
			queue = append(queue, storage.ChunkRef{Offset: offset, ChunkID: chunk.ID})
		}
		return nil
	})

	sort.Slice(queue, func(i, j int) bool {
		return queue[i].Offset < queue[j].Offset
	})

	// We know tree == ID for a new snapshot.
	err = scc.st.CopyChunkRefs(ctx, scc.request.TargetID, scc.request.TargetID, queue)
	if err != nil {
		return err
	}

	err = scc.st.EndSnapshot(ctx, scc.request.TargetID)
	if err != nil {
		return err
	}

	checksum, err := chunker.NewCalculateContext(scc.st, scc.request.TargetID).UpdateChecksum(ctx)
	if err != nil {
		return err
	}

	if sh.Checksum != checksum {
		log.G(ctx).Error("ShallowCopy.End: checksum mismatch",
			zap.String("source", sh.Checksum), zap.String("destination", checksum))
		return misc.ErrSnapshotCorrupted
	}

	return nil
}

func (scc *ShallowCopyContext) finish(ctx context.Context, err error) {
	scc.mutex.Lock()
	scc.finished = true
	scc.err = err
	t := time.Now()
	scc.finishedAt = &t
	scc.mutex.Unlock()
	log.G(ctx).Debug("ShallowCopyContext finished", zap.Error(err))
}
