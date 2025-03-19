package lib

import (
	"fmt"
	"io"
	"net"
	"net/http"
	"os"
	"path/filepath"
	"strings"

	"go.uber.org/zap"
	"golang.org/x/net/context"
	"golang.org/x/xerrors"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/go-common/mon"
	"a.yandex-team.ru/cloud/compute/go-common/pkg/metriclabels"
	"a.yandex-team.ru/cloud/compute/go-common/tracing"
	"a.yandex-team.ru/cloud/compute/snapshot/internal/dockerprocess"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/config"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/chunker"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/convert"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/filewatcher"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/kikimr"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/move"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/parallellimiter"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/proxy"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/storage"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/tasks"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/logging"
)

const (
	kikimrGrpc = "kikimr+grpc"
)

// Snapshot is a handle to a snapshot
type Snapshot struct {
	id string
	si *common.SnapshotInfo
	f  *Facade
}

// Facade is an entry point for package users.
// All snapshot-related operations must be done by calling its methods.
type Facade struct {
	config *config.Config
	st     storage.Storage
	c      *convert.Converter
	reg    convert.ImageSourceRegistry
	mcf    *tasks.MoveContextFactory
	tm     *tasks.TaskManager
	pr     *proxy.Proxy

	labelsFile filewatcher.Watcher
}

func PrepareStorage(ctx context.Context, conf *config.Config, tableOps int) (storage.Storage, error) {
	switch conf.General.Storage {
	case kikimrGrpc:
		return kikimr.NewKikimr(ctx, conf, tableOps)
	default:
		err := fmt.Errorf("invalid value for Storage: '%s'", conf.General.Storage)
		log.G(ctx).Error("PrepareStorage failed", zap.String("storage", conf.General.Storage), zap.Error(err))
		return nil, err
	}
}

// NewFacade creates new Facade according to conf.
func NewFacade(ctx context.Context, conf *config.Config) (*Facade, error) {
	return NewFacadeOps(ctx, conf, misc.TableOpNone)
}

// NewFacadeOps creates new Facade according to conf
// optionally clearing databases.
func NewFacadeOps(ctx context.Context, conf *config.Config, tableOps int) (*Facade, error) {
	st, err := PrepareStorage(ctx, conf, tableOps)
	if err != nil {
		return nil, err
	}

	var c *convert.Converter
	var pr *proxy.Proxy
	if tableOps&misc.TableOpDBOnly == 0 {
		c, err = convert.NewConverter(ctx, conf)
		if err != nil {
			return nil, err
		}

		err = os.Remove(conf.QemuDockerProxy.SocketPath)
		log.G(ctx).Debug("Remove old socket", zap.String("path", conf.QemuDockerProxy.SocketPath), zap.Error(err))

		l, err := net.Listen("unix", conf.QemuDockerProxy.SocketPath)
		if err != nil {
			log.G(ctx).Error("failed to create proxy listener", zap.Error(err))

			// debug log
			dir := filepath.Dir(conf.QemuDockerProxy.SocketPath)
			stat, errStat := os.Stat(dir)
			log.DebugErrorCtx(ctx, errStat, "Stat socket dir", zap.Any("stat", stat), zap.String("dir", dir))
			return nil, err
		}

		pr = proxy.NewProxy(conf.QemuDockerProxy.HostForDocker, conf.QemuDockerProxy.IDLengthBytes)
		go func() {
			err := pr.Serve(l)
			if err != http.ErrServerClosed {
				log.G(ctx).Panic("proxy not started properly", zap.Error(err))
			}
		}()
	}

	var reg convert.ImageSourceRegistry
	// "Dummy" url source must be before S3 to avoid warnings
	if conf.S3.Dummy {
		reg = append(reg, convert.HTTPSource{
			EnableRedirects: conf.S3.EnableRedirects,
			ProxySock:       conf.QemuDockerProxy.SocketPath,
		})
	}
	reg = append(reg, convert.NewS3(&conf.S3, conf.QemuDockerProxy.SocketPath))

	var w filewatcher.Watcher
	if conf.General.MetricDetailsFile == "" {
		w = filewatcher.NewNilWatcher()
	} else {
		if watcher, err := filewatcher.NewWatcher(ctx, conf.General.MetricDetailsFile); err == nil {
			w = watcher
		} else {
			return nil, err
		}
	}

	limiterSettings := parallellimiter.RateLimiterSettings{
		CheckInterval: conf.TaskLimiter.CheckInterval.Duration,
		MaxPerAZ: map[string]int{
			parallellimiter.OperationTypeTotal: conf.TaskLimiter.ZoneTotal,
			tasks.OperationTypeUnknown:         conf.TaskLimiter.ZoneUnknown,
			tasks.OperationTypeDelete:          conf.TaskLimiter.ZoneDelete,
			tasks.OperationTypeImport:          conf.TaskLimiter.ZoneImport,
			tasks.OperationTypeRestore:         conf.TaskLimiter.ZoneRestore,
			tasks.OperationTypeShallowCopy:     conf.TaskLimiter.ZoneShallowCopy,
			tasks.OperationTypeSnapshot:        conf.TaskLimiter.ZoneSnapshot,
		},
		MaxPerCloud: map[string]int{
			parallellimiter.OperationTypeTotal: conf.TaskLimiter.CloudTotal,
			tasks.OperationTypeDelete:          conf.TaskLimiter.CloudDelete,
			tasks.OperationTypeImport:          conf.TaskLimiter.CloudImport,
			tasks.OperationTypeRestore:         conf.TaskLimiter.CloudRestore,
			tasks.OperationTypeShallowCopy:     conf.TaskLimiter.CloudShallowCopy,
			tasks.OperationTypeSnapshot:        conf.TaskLimiter.CloudSnapshot,
		},
		MaxPerProject: map[string]int{
			parallellimiter.OperationTypeTotal: conf.TaskLimiter.ProjectTotal,
			tasks.OperationTypeDelete:          conf.TaskLimiter.ProjectDelete,
			tasks.OperationTypeImport:          conf.TaskLimiter.ProjectImport,
			tasks.OperationTypeRestore:         conf.TaskLimiter.ProjectRestore,
			tasks.OperationTypeShallowCopy:     conf.TaskLimiter.ProjectShallowCopy,
			tasks.OperationTypeSnapshot:        conf.TaskLimiter.ProjectSnapshot,
		},
		MaxPerSnapshot: map[string]int{
			parallellimiter.OperationTypeTotal: conf.TaskLimiter.SnapshotTotal,
			tasks.OperationTypeRestore:         conf.TaskLimiter.ProjectRestore,
			tasks.OperationTypeShallowCopy:     conf.TaskLimiter.ProjectShallowCopy,
		},
	}

	log.G(ctx).Info("Start with limiter settings", zap.Any("settings", limiterSettings))

	taskLimiter := parallellimiter.NewLimiter(st, limiterSettings)

	return &Facade{
		config: conf,
		st:     st,
		c:      c,
		reg:    reg,
		mcf:    tasks.NewMoveContextFactory(ctx, conf, move.NewDeviceFactory(st, conf, c, reg, conf.QemuDockerProxy.SocketPath), pr),
		tm:     tasks.NewTaskManager(taskLimiter, conf.General.ZoneID),
		pr:     pr,

		labelsFile: w,
	}, nil
}

// RegisterMon sets monitoring check
func (f *Facade) RegisterMon(r mon.Repository) error {
	r.Add("kikimr", mon.CheckFunc(f.st.Health))
	if f.c != nil {
		r.Add("nbd", mon.CheckFunc(f.c.Registry.Check))
	}

	r.Add("docker", mon.CheckFunc(func(ctx context.Context) error {
		example := "123"
		data, err := dockerprocess.Exec(ctx, []string{"echo", "-n", example}, nil)
		if string(data) != example {
			return fmt.Errorf(`"%s" should be "%s"`, data, example)
		}
		return err
	}))

	r.Add("tasks", mon.CheckFunc(f.tm.Check))

	return f.mcf.GetMDF().RegisterMon(r)
}

// CreateSnapshot creates a new snapshot with given metadata. Tests only
func (f *Facade) CreateSnapshot(ctx context.Context, d *common.CreationInfo) (*Snapshot, error) {
	if err := misc.FillIDIfAbsent(ctx, &d.ID); err != nil {
		return nil, err
	}

	ctx = log.WithLogger(ctx, log.G(ctx).With(
		logging.Method("create"), logging.Request(d),
		logging.SnapshotID(d.ID)))
	log.G(ctx).Info("facade: CreateSnapshot")

	id, err := f.st.BeginSnapshot(ctx, d)
	if err != nil {
		return nil, err
	}
	return &Snapshot{id: id, f: f}, nil
}

// OpenSnapshotWriteable returns a snapshot handle to which one can write.
func (f *Facade) OpenSnapshotWriteable(ctx context.Context, id string) (*Snapshot, error) {
	// TODO (arovesto) remove completely after grace period
	return &Snapshot{id: id, f: f}, nil
}

// OpenSnapshot returns a snapshot handle and checks that snapshot exists. (Handle is deprecated, used only for snapshot info)
func (f *Facade) OpenSnapshot(ctx context.Context, id string, skipStatusCheck bool) (sh *Snapshot, err error) {
	span, ctx := tracing.StartSpanFromContext(ctx, "facade-opensnapshot")
	defer func() { tracing.Finish(span, err) }()

	ctx = log.WithLogger(ctx, log.G(ctx).With(
		logging.Method("load"), logging.SnapshotID(id)))
	log.G(ctx).Info("facade: OpenSnapshot")

	var info *common.SnapshotInfo
	if skipStatusCheck {
		info, err = f.st.GetSnapshot(ctx, id)
	} else {
		info, err = f.st.GetLiveSnapshot(ctx, id)
	}
	if err != nil {
		return nil, err
	}
	return &Snapshot{id: id, si: info, f: f}, nil
}

// CommitSnapshot finishes snapshot creation.
// All non-filled chunks will be either treated as zero (in case of full snapshot)
// or inherited from base (in case of incremental snapshot).
func (f *Facade) CommitSnapshot(ctx context.Context, id string, waitChecksum bool) error {
	ctx = log.WithLogger(ctx, log.G(ctx).With(logging.Method("commit"), logging.SnapshotID(id)))
	log.G(ctx).Info("facade: CommitSnapshot")
	ctx, holder, err := f.st.LockSnapshot(ctx, id, "commit-snapshot")
	if err != nil {
		return xerrors.Errorf("lock snapshot for commit")
	}
	defer holder.Close(ctx)

	err = f.st.EndSnapshot(ctx, id)
	if err == nil {
		ctx = log.WithLogger(context.Background(), log.G(ctx))
		if waitChecksum {
			_, err = chunker.NewCalculateContext(f.st, id).UpdateChecksum(ctx)
			return err
		}
		go func() {
			_, err = chunker.NewCalculateContext(f.st, id).UpdateChecksum(ctx)
			log.DebugErrorCtx(ctx, err, "CommitSnapshot")
		}()
	}
	return err
}

func (f *Facade) deleteSnapshot(ctx context.Context, d *common.DeleteRequest) (deleteContext *tasks.DeleteContext, c context.Context, err error) {
	t := misc.DeleteSnapshot.Start(ctx)
	defer t.ObserveDuration()
	// For HTTP compatibility, we allow delete without task.
	if err := misc.FillIDIfAbsent(ctx, &d.Params.TaskID); err != nil {
		return nil, ctx, err
	}

	ctx = log.WithLogger(ctx, log.G(ctx).With(
		logging.Method("delete"), logging.SnapshotID(d.ID),
		logging.TaskID(d.Params.TaskID)))
	log.G(ctx).Info("facade: DeleteSnapshot")

	dc := tasks.NewDeleteContext(d, f.st)
	err = dc.Begin(ctx)
	if err != nil {
		return dc, ctx, err
	}

	ctx = misc.WithNoCancel(ctx)
	return dc, ctx, nil
}

// DeleteSnapshot marks snapshot as deleted and starts an async cleanup operation.
func (f *Facade) DeleteSnapshot(ctx context.Context, d *common.DeleteRequest) (err error) {
	if dc, c, err := f.deleteSnapshot(ctx, d); err != nil {
		return err
	} else {
		return f.tm.Run(c, dc)
	}
}

// ListSnapshots returns a list of snapshots matching the request.
func (f *Facade) ListSnapshots(ctx context.Context, r *storage.ListRequest) ([]common.SnapshotInfo, error) {
	ctx = log.WithLogger(ctx, log.G(ctx).With(
		logging.Method("list"), logging.Request(r)))
	log.G(ctx).Info("facade: ListSnapshots")

	return f.st.List(ctx, r)
}

// ListSnapshotBases returns a list of snapshot bases from latest to first.
func (f *Facade) ListSnapshotBases(ctx context.Context, r *storage.BaseListRequest) ([]common.SnapshotInfo, error) {
	ctx = log.WithLogger(ctx, log.G(ctx).With(
		logging.Method("list-bases"), logging.Request(r)))
	log.G(ctx).Info("facade: ListSnapshotBases")

	return f.st.ListBases(ctx, r)
}

// ListSnapshotChangedChildren returns a list of child size changes on base deletion.
func (f *Facade) ListSnapshotChangedChildren(ctx context.Context, id string) ([]common.ChangedChild, error) {
	ctx = log.WithLogger(ctx, log.G(ctx).With(
		logging.Method("list-changed-children"), logging.SnapshotID(id)))
	log.G(ctx).Info("facade: ListSnapshotChangedChildren")

	return f.st.ListChangedChildren(ctx, id)
}

// ConvertSnapshot starts an async conversion operation.
func (f *Facade) ConvertSnapshot(ctx context.Context, r *common.ConvertRequest) (string, string, error) {
	ctx = log.WithLogger(ctx, log.G(ctx).With(
		logging.Method("convert"), logging.Request(r)))
	log.G(ctx).Info("facade: ConvertSnapshot")

	if err := misc.FillIDIfAbsent(ctx, &r.ID); err != nil {
		return "", "", err
	}
	var taskID string
	if err := misc.FillIDIfAbsent(ctx, &taskID); err != nil {
		return "", "", err
	}

	mr := common.MoveRequest{
		OperationProjectID: r.ProjectID,
		Src: &common.URLMoveSrc{
			Format: r.Format,
			Bucket: r.Bucket,
			Key:    r.Key,
			URL:    r.URL,
		},
		Dst: &common.SnapshotMoveDst{
			SnapshotID: r.ID,
			ProjectID:  r.ProjectID,
			Create:     true,
			Commit:     true,
			Fail:       true,
			Name:       r.Name,
			ImageID:    r.ImageID,
		},
		Params: common.MoveParams{
			TaskID:     taskID,
			SkipZeroes: true,
		},
	}

	err := f.Move(ctx, &mr)
	if err != nil {
		return "", "", err
	}
	return r.ID, taskID, nil
}

// GetSnapshotChunksWithOffset returns a batch of subsequent chunks with offset greater than given.
func (f *Facade) GetSnapshotChunksWithOffset(ctx context.Context, id string, offset int64, limit int) ([]common.ChunkInfo, error) {
	ctx = log.WithLogger(ctx, log.G(ctx).With(
		logging.Method("get-map"), logging.SnapshotID(id),
		logging.SnapshotOffset(offset), zap.Int("limit", limit)))
	log.G(ctx).Info("facade: GetSnapshotChunksWithOffset")
	// TODO (arovesto) remove completely after grace period
	return f.getChunker().GetSnapshotChunksWithOffset(ctx, id, offset, limit)
}

// ReadChunkBody returns chunk data.
func (f *Facade) ReadChunkBody(ctx context.Context, id string, offset int64) ([]byte, error) {
	ctx = log.WithLogger(ctx, log.G(ctx).With(
		logging.Method("read-body"), logging.SnapshotID(id),
		zap.Int64("offset", offset)))
	log.G(ctx).Info("facade: ReadChunkBody")
	// TODO (arovesto) remove completely after grace period
	return f.getChunker().ReadChunkBody(ctx, id, offset)
}

// UpdateSnapshot updates specified fields in snapshot metadata.
func (f *Facade) UpdateSnapshot(ctx context.Context, r *common.UpdateRequest) error {
	ctx = log.WithLogger(ctx, log.G(ctx).With(
		logging.Method("update"), logging.SnapshotID(r.ID),
		logging.Request(r)))
	log.G(ctx).Info("facade: UpdateSnapshot")

	return f.st.UpdateSnapshot(ctx, r)
}

// DeleteTombstone clears snapshot in 'deleted' state
func (f *Facade) DeleteTombstone(ctx context.Context, id string) error {
	ctx = log.WithLogger(ctx, log.G(ctx).With(
		logging.Method("delete-tombstone"), logging.SnapshotID(id)))
	log.G(ctx).Info("facade: DeleteTombstone")

	return f.st.DeleteTombstone(ctx, id)
}

// VerifyChecksum verifies snapshot and per-chunk checksums
func (f *Facade) VerifyChecksum(ctx context.Context, id string, full bool) (*common.VerifyReport, error) {
	ctx = log.WithLogger(ctx, log.G(ctx).With(
		logging.Method("verify-checksum"), logging.SnapshotID(id)))
	log.G(ctx).Info("facade: VerifyChecksum")

	return chunker.NewVerifyContext(f.st, f.config.Performance.VerifyWorkers, id, full).VerifyChecksum(ctx)
}

// ShallowCopy creates a new snapshot with by referencing other's blocks
func (f *Facade) ShallowCopy(ctx context.Context, r *common.CopyRequest) (string, error) {
	if err := misc.FillIDIfAbsent(ctx, &r.TargetID); err != nil {
		return "", err
	}
	if err := misc.FillIDIfAbsent(ctx, &r.Params.TaskID); err != nil {
		return "", err
	}

	ctx = log.WithLogger(ctx, log.G(ctx).With(
		logging.Method("shallow-copy"), logging.SnapshotID(r.TargetID),
		logging.TaskID(r.Params.TaskID), logging.Request(r)))
	log.G(ctx).Info("facade: ShallowCopy")

	scc := tasks.NewShallowCopyContext(f.st, r)
	err := scc.Begin(ctx)
	if err != nil {
		return "", err
	}

	ctx = misc.WithNoCancel(ctx)
	return r.TargetID, f.tm.Run(ctx, scc)
}

// Move starts a data moving task
func (f *Facade) Move(ctx context.Context, r *common.MoveRequest) (resErr error) {
	span, ctx := tracing.InitSpan(ctx, "facade-move")
	defer func() { tracing.Finish(span, resErr) }()

	ctx = log.WithLogger(ctx, log.G(ctx).With(
		logging.Method("move"), zap.String("src", r.Src.Description()),
		zap.String("dst", r.Dst.Description()), logging.TaskID(r.Params.TaskID)))
	log.G(ctx).Info("facade: Move")

	labels := metriclabels.Get(ctx)
	labeledFeatures := f.labelsFile.GetString()

	var projectID, snapshotID string

	projectID = r.OperationProjectID
	log.G(ctx).Debug("Set operation project id from request", zap.String("project_id", projectID))

	switch src := r.Src.(type) {
	case *common.SnapshotMoveSrc:
		if projectID == "" {
			info, err := f.st.GetSnapshot(ctx, src.SnapshotID)
			log.DebugErrorCtx(ctx, err, "Get snapshot info")
			if err == nil {
				projectID = info.ProjectID
				log.G(ctx).Debug("Set operation project id from snapshot src", zap.String("project_id", projectID))
			}
		}
		snapshotID = src.SnapshotID
	}

	switch dst := r.Dst.(type) {
	case *common.SnapshotMoveDst:
		if (dst.ProjectID != "" && strings.Contains(labeledFeatures, dst.ProjectID)) ||
			strings.Contains(labeledFeatures, dst.SnapshotID) {
			labels.DetailLabels = true
		}
		if projectID == "" {
			projectID = dst.ProjectID
			log.G(ctx).Debug("Set operation project id from snapshot dst request", zap.String("project_id", projectID))
		}
		snapshotID = dst.SnapshotID

		if projectID == "" {
			info, err := f.st.GetSnapshot(ctx, dst.SnapshotID)
			if err == nil {
				projectID = info.ProjectID
				log.G(ctx).Debug("Set operation project id from snapshot dst database", zap.String("project_id", projectID))
			}

			if err != misc.ErrSnapshotNotFound {
				log.DebugErrorCtx(ctx, err, "Get snapshot info in facade.Move")
			}
			if err != misc.ErrSnapshotNotFound {
				log.DebugErrorCtx(ctx, err, "Get snapshot info")
			}
		}

	case *common.NbsMoveDst:
		if strings.Contains(labeledFeatures, dst.DiskID) {
			labels.DetailLabels = true
		}
	}

	if src, ok := r.Src.(*common.URLMoveSrc); ok && strings.Contains(labeledFeatures, src.URL) {
		labels.DetailLabels = true
	}

	mc := f.mcf.GetMoveContext(ctx, r, projectID, snapshotID)
	ctx = misc.WithNoCancel(ctx)
	ctx = log.WithLogger(metriclabels.WithMetricData(ctx, labels), log.G(ctx))
	return f.tm.Run(ctx, mc)
}

func (f *Facade) ListTasks(ctx context.Context) []*common.TaskInfo {
	ctx = log.WithLogger(ctx, log.G(ctx).With(logging.Method("list-task")))
	log.G(ctx).Info("facade: ListTask")

	return f.tm.ListTasks()
}

func (f *Facade) GetTask(ctx context.Context, taskID string) (*common.TaskData, error) {
	ctx = log.WithLogger(ctx, log.G(ctx).With(logging.Method("get-task"), logging.TaskID(taskID)))
	log.G(ctx).Info("facade: GetTask")

	taskContext := f.tm.Get(taskID)
	if taskContext == nil {
		return nil, misc.ErrTaskNotFound
	}
	return &common.TaskData{
		Status:  *taskContext.GetStatus(),
		Request: taskContext.GetRequest(),
	}, nil

}

// GetTaskStatus returns status of (possible finished) task
func (f *Facade) GetTaskStatus(ctx context.Context, taskID string) (*common.TaskStatus, error) {
	ctx = log.WithLogger(ctx, log.G(ctx).With(
		logging.Method("get-task-status"), logging.TaskID(taskID)))
	log.G(ctx).Debug("facade: GetTaskStatus start")

	taskContext := f.tm.Get(taskID)
	if taskContext == nil {
		return nil, misc.ErrTaskNotFound
	}

	status := taskContext.GetStatus()
	log.G(ctx).Info("facade: GetTaskStatus", zap.Reflect("status", status))

	return status, nil
}

// CancelTask interrupts a task without clearing information
func (f *Facade) CancelTask(ctx context.Context, taskID string) error {
	ctx = log.WithLogger(ctx, log.G(ctx).With(
		logging.Method("cancel-task"), logging.TaskID(taskID)))
	log.G(ctx).Info("facade: CancelTask")

	taskContext := f.tm.Get(taskID)
	if taskContext == nil {
		return misc.ErrTaskNotFound
	}

	taskContext.Cancel(ctx)
	return nil
}

// DeleteTask deletes task information and cancels it if needed
func (f *Facade) DeleteTask(ctx context.Context, taskID string) error {
	ctx = log.WithLogger(ctx, log.G(ctx).With(
		logging.Method("delete-task"), logging.TaskID(taskID)))
	log.G(ctx).Info("facade: DeleteTask")

	taskContext := f.tm.Get(taskID)
	if taskContext == nil {
		return misc.ErrTaskNotFound
	}

	taskContext.Cancel(ctx)

	ok := f.tm.Delete(taskID)
	if !ok {
		return misc.ErrTaskNotFound
	}

	return nil
}

// Close closes the Facade and releases all system-related resources.
func (f *Facade) Close(ctx context.Context) error {
	if f.c != nil {
		f.c.Close(ctx)
	}
	if f.mcf != nil {
		_ = f.mcf.Close(ctx)
	}
	if f.pr != nil {
		err := f.pr.Stop()
		if err != nil {
			log.G(ctx).Error("error during proxy stop", zap.Error(err))
		}
	}

	if f.pr != nil {
		err := os.Remove(f.config.QemuDockerProxy.SocketPath)
		log.DebugErrorCtx(ctx, err, "Remove proxy socket")
	}

	f.tm.Close(ctx)
	f.labelsFile.Close()
	return f.st.Close()
}

func (f *Facade) getChunker() *chunker.Chunker {
	return chunker.NewChunker(f.st, chunker.MustBuildHasher(f.config.General.ChecksumAlgorithm), chunker.NewLZ4Compressor)
}

// Write stores a chunk to snapshot.
func (sh *Snapshot) Write(ctx context.Context, c *chunker.StreamChunk) error {
	ctx = log.WithLogger(ctx, log.G(ctx).With(
		logging.Method("write"), logging.SnapshotID(sh.id)))
	log.G(ctx).Info("facade: Snapshot.Write", logging.SnapshotOffset(c.Offset))

	// TODO (arovesto) remove completely after grace period
	return sh.f.getChunker().StoreChunk(ctx, sh.id, c)
}

// Read returns a reader to read snapshot data starting from offset.
// Deprecated.
func (sh *Snapshot) Read(ctx context.Context, offset int64) (io.ReadCloser, error) {
	ctx = log.WithLogger(ctx, log.G(ctx).With(
		logging.Method("read"), logging.SnapshotID(sh.id),
		zap.Int64("offset", offset)))
	log.G(ctx).Info("facade: Snapshot.Read")

	// TODO (arovesto) remove completely after grace period
	return sh.f.getChunker().GetReader(ctx, sh.id, offset)
}

// GetID returns snapshot ID.
func (sh *Snapshot) GetID() string {
	return sh.id
}

// GetInfo returns snapshot metadata.
func (sh *Snapshot) GetInfo() *common.SnapshotInfo {
	return sh.si
}
