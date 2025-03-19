package tasks

import (
	"crypto/md5"
	"fmt"
	"sync"
	"time"

	"a.yandex-team.ru/cloud/compute/go-common/util"

	"github.com/prometheus/client_golang/prometheus"
	"go.uber.org/zap"
	"golang.org/x/net/context"
	"golang.org/x/sync/errgroup"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/go-common/pkg/metriclabels"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/storage"
)

var _ Task = &DeleteContext{}

const OperationTypeDelete = "delete"

// DeleteContext implements the deletion task.
type DeleteContext struct {
	request   *common.DeleteRequest
	st        storage.Storage
	projectID string

	contextHeartbeat

	group         *errgroup.Group
	groupMixinCtx context.Context

	mutex    sync.Mutex
	finished bool
	err      error

	createdAt  time.Time
	finishedAt *time.Time
}

// NewDeleteContext returns a new DeleteContext instance.
func NewDeleteContext(request *common.DeleteRequest, st storage.Storage) *DeleteContext {
	return &DeleteContext{
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
func (dc *DeleteContext) GetID() string {
	return dc.request.Params.TaskID
}

// GetStatus returns task status.
func (dc *DeleteContext) GetStatus() *common.TaskStatus {
	if dc.request.Params.HeartbeatEnabled {
		// To update heartbeat timer.
		select {
		case <-dc.statusChan: // pass
		default: // pass
		}
	}

	dc.mutex.Lock()
	defer dc.mutex.Unlock()

	return &common.TaskStatus{
		Finished:   dc.finished,
		Success:    dc.err == nil,
		Error:      dc.err,
		CreatedAt:  dc.createdAt,
		FinishedAt: dc.finishedAt,
	}
}

// Cancel cancels the task.
func (dc *DeleteContext) Cancel(ctx context.Context) {
	lock := util.MakeLockGuard(&dc.mutex)
	lock.Lock()
	defer lock.UnlockIfLocked()

	if dc.cancel == nil {
		return
	}

	if !dc.finished {
		dc.finished = true
		dc.err = misc.ErrTaskCancelled
	}
	lock.Unlock()

	log.G(ctx).Debug("Canceling groupCtx due to end of DeleteContext.Cancel")
	dc.cancel()
	_ = dc.group.Wait()
}

func (dc *DeleteContext) Timer(ctx context.Context) *prometheus.Timer {
	return misc.EndDeleteSnapshotTimer.Start(ctx)
}

// Begin is a sync part of snapshot deletion.
func (dc *DeleteContext) Begin(ctx context.Context) error {
	if dc.request.OperationProjectID != "" {
		dc.projectID = dc.request.OperationProjectID
		log.G(ctx).Debug("Set delete operation project id from request", zap.String("project_id", dc.projectID))
	}
	if dc.request.SkipStatusCheck {
		if info, err := dc.st.GetSnapshot(ctx, dc.request.ID); err == nil {
			if dc.projectID == "" {
				dc.projectID = info.ProjectID
				log.G(ctx).Debug("Set delete operation project id from snapshot", zap.String("project_id", dc.projectID))
			}
		} else {
			return err
		}
	} else {
		if info, err := dc.st.GetLiveSnapshot(ctx, dc.request.ID); err == nil {
			if dc.projectID == "" {
				dc.projectID = info.ProjectID
				log.G(ctx).Debug("Set delete operation project id from live snapshot", zap.String("project_id", dc.projectID))
			}
		} else {
			return err
		}
	}

	if err := dc.st.BeginDeleteSnapshot(ctx, dc.request.ID); err != nil && err != misc.ErrSnapshotNotFound {
		return err
	}
	return nil
}

func (dc *DeleteContext) RequestHash() string {
	return fmt.Sprintf("%x", md5.Sum([]byte(fmt.Sprintf("%v", dc.request))))
}

func (dc *DeleteContext) CheckHeartbeat(err error) error {
	return dc.checkHeartbeat(err)
}

func (dc *DeleteContext) GetHeartbeat() time.Duration {
	return time.Duration(dc.request.Params.HeartbeatTimeoutMs) * time.Millisecond
}

func (dc *DeleteContext) Type() string {
	return OperationTypeDelete
}

func (dc *DeleteContext) GetOperationCloudID() string {
	return dc.request.OperationCloudID
}

func (dc *DeleteContext) GetOperationProjectID() string {
	return dc.projectID
}

func (dc *DeleteContext) GetSnapshotID() string {
	return dc.request.ID
}

func (dc *DeleteContext) GetRequest() common.TaskRequest {
	return dc.request
}

func (dc *DeleteContext) InitTaskWorkContext(ctx context.Context) context.Context {
	ctx, dc.cancel = context.WithCancel(ctx)
	if dc.request.Params.HeartbeatEnabled {
		dc.startHeartbeat(ctx)
	}
	dc.group, dc.groupMixinCtx = errgroup.WithContext(context.Background())
	return ctx
}

// End starts the task.
func (dc *DeleteContext) DoWork(ctx context.Context) (resErr error) {
	defer func() {
		log.G(ctx).Debug("Canceling ctx due to end of DeleteContext.DoWork")
		dc.cancel()
		resErr = dc.checkHeartbeat(resErr)
	}()

	ctx = metriclabels.WithMetricData(ctx, metriclabels.Get(ctx))

	groupCtx := misc.CompoundContext(ctx, dc.groupMixinCtx)

	dc.group.Go(func() error {
		return dc.st.EndDeleteSnapshotSync(groupCtx, dc.request.ID, true)
	})

	err := dc.group.Wait()
	if dc.request.Params.HeartbeatEnabled {
		// If heartbeat elapsed, we will have wrong err.
		err = dc.checkHeartbeat(err)
	}

	dc.finish(ctx, err)
	return err
}

func (dc *DeleteContext) finish(ctx context.Context, err error) {
	dc.mutex.Lock()
	dc.finished = true
	dc.err = err
	t := time.Now()
	dc.finishedAt = &t
	dc.mutex.Unlock()
	log.G(ctx).Debug("DeleteContext finished", zap.Error(err))
}
