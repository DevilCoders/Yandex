package tasks

import (
	"context"
	"crypto/md5"
	"fmt"
	"io"
	"sync"
	"time"

	"github.com/opentracing/opentracing-go"
	otlog "github.com/opentracing/opentracing-go/log"
	"github.com/prometheus/client_golang/prometheus"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
	"golang.org/x/sync/errgroup"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/go-common/pkg/metriclabels"
	"a.yandex-team.ru/cloud/compute/go-common/tracing"
	"a.yandex-team.ru/cloud/compute/go-common/util"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/config"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/move"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/proxy"
)

var _ Task = &MoveContext{}

// we need to store still not store batch of chunks here + start clear process if required so
const nbsTimeout = 30 * time.Second

const (
	OperationTypeImport   = "import"
	OperationTypeSnapshot = "snapshot"
	OperationTypeRestore  = "restore"
)

// MoveContextFactory creates MoveContext's.
type MoveContextFactory struct {
	mdf          *move.DeviceFactory
	workersCount int
	Proxy        *proxy.Proxy
}

// NewMoveContextFactory returns new MoveContextFactory instance.
func NewMoveContextFactory(ctx context.Context, conf *config.Config, mdf *move.DeviceFactory, pr *proxy.Proxy) *MoveContextFactory {
	log.G(ctx).Info("Init MoveContextFactory.", zap.Int("workersCount", conf.Performance.MoveWorkers))
	return &MoveContextFactory{
		mdf:          mdf,
		workersCount: conf.Performance.MoveWorkers,
		Proxy:        pr,
	}
}

// GetMDF returns internal device factory,
func (mcf *MoveContextFactory) GetMDF() *move.DeviceFactory {
	return mcf.mdf
}

// GetMoveContext builds a new MoveContext instance.
func (mcf *MoveContextFactory) GetMoveContext(ctx context.Context, request *common.MoveRequest, projectID, snapshotID string) *MoveContext {
	return &MoveContext{
		request: request,

		projectID:  projectID,
		snapshotID: snapshotID,

		mcf: mcf,
		contextHeartbeat: contextHeartbeat{
			timeoutMs:  request.Params.HeartbeatTimeoutMs,
			statusChan: make(chan struct{}),
		},
		flushedOffset: -1,
		createdAt:     time.Now(),
	}
}

// Close releases used resources.
func (mcf *MoveContextFactory) Close(ctx context.Context) error {
	return mcf.mdf.Close(ctx)
}

type contextHeartbeat struct {
	timeoutMs  int32
	statusChan chan struct{}
	cancel     context.CancelFunc
	hbLock     sync.Mutex
	hbErr      error
}

func (ch *contextHeartbeat) startHeartbeat(ctx context.Context) {
	go func() {
		err := ch.heartbeat(ctx)
		if err != nil {
			ch.hbLock.Lock()
			ch.hbErr = err
			ch.hbLock.Unlock()
		}
		// We can close channel because only finished heartbeat() writes to it
		close(ch.statusChan)
		// Interrupt the workers
		log.DebugErrorCtx(ctx, err, "Canceling heartbeatCtx due to heartbeat ended (maybe with error)")
		ch.cancel()
	}()
}

func (ch *contextHeartbeat) heartbeat(ctx context.Context) error {
	timeout := time.Duration(ch.timeoutMs) * time.Millisecond
	for {
		select {
		case ch.statusChan <- struct{}{}:
		case <-time.After(timeout):
			// We are lost. Harakiri!
			log.G(ctx).Warn("Heartbeat timeout elapsed, interrupting", zap.Duration("timeout", timeout))
			return misc.ErrHeartbeatTimeout
		case <-ctx.Done():
			return ctx.Err()
		}
	}
}

func (ch *contextHeartbeat) stopHeartbeat(ctx context.Context, err error) error {
	// Notify heartbeat routine
	log.G(ctx).Debug("Canceling heartbeatCtx due to stopHeartbeat", zap.NamedError("cancel_with_error", err))
	ch.cancel()
	return ch.checkHeartbeat(err)
}

func (ch *contextHeartbeat) checkHeartbeat(err error) error {
	ch.hbLock.Lock()
	// heartbeat() may return misc.ErrHeartbeatTimeout only
	if ch.hbErr != nil {
		err = ch.hbErr
	}
	ch.hbLock.Unlock()
	return err
}

// MoveContext is a data move task.
type MoveContext struct {
	// Creation params
	request *common.MoveRequest
	mcf     *MoveContextFactory
	contextHeartbeat

	projectID  string
	snapshotID string

	// Init-set params
	src          move.Src
	dst          move.Dst
	workersCount int
	blockSize    int64
	size         int64
	pool         sync.Pool
	observer     prometheus.Observer
	timer        *prometheus.Timer

	group         *errgroup.Group
	groupMixinCtx context.Context

	// Worker shared state && status
	mutex         sync.Mutex
	finished      bool
	err           error
	nextOffset    int64
	currentOffset map[int]int64
	flushedOffset int64

	createdAt  time.Time
	finishedAt *time.Time
}

// GetID returns task ID.
func (mc *MoveContext) GetID() string {
	return mc.request.Params.TaskID
}

func (mc *MoveContext) flush(ctx context.Context) error {
	flushable, ok := mc.dst.(move.Flushable)
	if !ok {
		return nil
	}
	ctx = log.WithLogger(ctx, log.G(ctx).Named("MoveContext Flushable"))

	for {
		select {
		case <-time.After(time.Second):
			mc.mutex.Lock()
			offset := mc.getResumeOffset()
			mc.mutex.Unlock()

			if err := flushable.Flush(ctx); err != nil {
				log.G(ctx).Error("move.flush failed with error", zap.Error(err))
				mc.cancel()
				return err
			}

			mc.mutex.Lock()
			mc.flushedOffset = offset
			mc.mutex.Unlock()

		case <-ctx.Done():
			return ctx.Err()
		}
	}
}

// should be called in mutex
func (mc *MoveContext) getResumeOffset() (resumeOffset int64) {
	if len(mc.currentOffset) == 0 {
		resumeOffset = mc.nextOffset
	} else {
		// Next processed offset is minimum of offsets being processed
		minOffset := int64(-1)
		for _, v := range mc.currentOffset {
			if minOffset < 0 || v < minOffset {
				minOffset = v
			}
		}
		resumeOffset = minOffset
	}
	return
}

// GetStatus returns task status.
func (mc *MoveContext) GetStatus() *common.TaskStatus {
	if mc.request.Params.HeartbeatEnabled {
		// To update heartbeat timer.
		select {
		case <-mc.statusChan: // pass
		default: // pass
		}
	}

	mc.mutex.Lock()
	defer mc.mutex.Unlock()

	resumeOffset := mc.getResumeOffset()
	if mc.flushedOffset != -1 {
		resumeOffset = mc.flushedOffset
	}

	var progress float64
	if mc.size > 0 {
		progress = float64(resumeOffset) / float64(mc.size)
	}

	return &common.TaskStatus{
		Finished:   mc.finished,
		Success:    mc.err == nil,
		Progress:   progress,
		Offset:     resumeOffset,
		Error:      mc.err,
		CreatedAt:  mc.createdAt,
		FinishedAt: mc.finishedAt,
	}
}

// Cancel cancels data moving.
func (mc *MoveContext) Cancel(ctx context.Context) {
	lock := util.MakeLockGuard(&mc.mutex)
	lock.Lock()
	defer lock.UnlockIfLocked()

	if mc.cancel == nil {
		return
	}

	if !mc.finished {
		mc.finished = true
		mc.err = misc.ErrTaskCancelled
	}

	lock.Unlock()

	log.G(ctx).Debug("Canceling heartbeatCtx by MoveContext.Cancel")
	mc.cancel()

	_ = mc.group.Wait()
}

func (mc *MoveContext) Timer(ctx context.Context) *prometheus.Timer {
	return misc.EndMoveTimer.Start(ctx)
}

func (mc *MoveContext) RequestHash() string {
	// new task with different offset and same other params considered as new task
	// Params are copied to avoid data race
	r := *mc.request
	r.Params.Offset = 0
	if src, ok := r.Src.(*common.URLMoveSrc); ok {
		src.ShadowedURL = ""
	}
	// MoveDst and MoveSrc interfaces implements fmt.Stringer, so %#v used to print stuct as is
	return fmt.Sprintf("%x", md5.Sum([]byte(fmt.Sprintf("%v", r))))
}

func (mc *MoveContext) CheckHeartbeat(err error) error {
	return mc.checkHeartbeat(err)
}

func (mc *MoveContext) GetHeartbeat() time.Duration {
	return time.Duration(mc.request.Params.HeartbeatTimeoutMs) * time.Millisecond
}

func (mc *MoveContext) Type() string {
	_, nbsSrc := mc.request.Src.(*common.NbsMoveSrc)
	_, snapshotSrc := mc.request.Src.(*common.SnapshotMoveSrc)
	_, urlSrc := mc.request.Src.(*common.URLMoveSrc)

	_, nbsDst := mc.request.Dst.(*common.NbsMoveDst)
	_, snapshotDst := mc.request.Dst.(*common.SnapshotMoveDst)

	switch {
	case urlSrc && snapshotDst:
		return OperationTypeImport
	case snapshotSrc && nbsDst:
		return OperationTypeRestore
	case nbsSrc && snapshotDst:
		return OperationTypeSnapshot
	default:
		return OperationTypeUnknown
	}
}

func (mc *MoveContext) GetOperationCloudID() string {
	return mc.request.OperationCloudID
}

func (mc *MoveContext) GetOperationProjectID() string {
	return mc.projectID
}

func (mc *MoveContext) GetSnapshotID() string {
	return mc.snapshotID
}

func (mc *MoveContext) GetRequest() common.TaskRequest {
	return mc.request
}

func (mc *MoveContext) InitTaskWorkContext(ctx context.Context) context.Context {
	var heartbeatCtx context.Context

	// Used to cancel tasks due to API call or missed heartbeats.
	heartbeatCtx, mc.cancel = context.WithCancel(ctx)

	if mc.request.Params.HeartbeatEnabled {
		mc.startHeartbeat(heartbeatCtx)
	}
	mc.group, mc.groupMixinCtx = errgroup.WithContext(context.Background())
	return heartbeatCtx
}

// End starts data moving task.
func (mc *MoveContext) DoWork(ctx context.Context) (resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	defer func() {
		log.G(ctx).Debug("Canceling heartbeatCtx due to end of MoveContext.DoWork()")
		mc.cancel()
		resErr = mc.checkHeartbeat(resErr)
	}()

	if src, ok := mc.request.Src.(*common.URLMoveSrc); ok {
		u, id, err := mc.mcf.Proxy.AddURL(src.URL)
		if err != nil {
			mc.finish(ctx, err)
			return err
		}
		log.G(ctx).Debug("id acquired", zap.String("url", src.URL), zap.String("id", id))
		src.ShadowedURL = u
		defer func() {
			if err := mc.mcf.Proxy.RemoveID(id); err != nil {
				log.G(ctx).Error("error on removing id", zap.Error(err), zap.String("id", id))
			} else {
				log.G(ctx).Debug("id remove successful", zap.String("id", id))
			}
		}()
	}

	groupCtx := misc.CompoundContext(ctx, mc.groupMixinCtx)
	err := mc.init(groupCtx)
	if err != nil {
		if mc.request.Params.HeartbeatEnabled {
			// If heartbeat elapsed, we will have wrong err.
			err = mc.stopHeartbeat(ctx, err)
		}
		// init() has already cleaned up everything
		mc.finish(ctx, err)
		return err
	}

	for i := 0; i < mc.workersCount; i++ {
		workerID := i
		mc.group.Go(func() error {
			return mc.copy(groupCtx, workerID)
		})
	}

	// we need to wait for flusher to stop after all workers are done
	g := sync.WaitGroup{}
	g.Add(1)
	go func() {
		defer g.Done()
		// this context is canceled when all workers are done
		log.DebugErrorCtx(ctx, mc.flush(groupCtx), "mc.flush done")
	}()

	err = mc.group.Wait()
	g.Wait()
	if mc.request.Params.HeartbeatEnabled {
		// Check error without stopping heartbeats.
		// This is needed to interrupt hanging Close().
		// If heartbeat elapsed, we will have wrong err.
		err = mc.checkHeartbeat(err)
	}
	if err == nil {
		mc.mutex.Lock()
		err = mc.err
		mc.mutex.Unlock()
	}
	log.G(ctx).Debug("Moving finished, closing devices", zap.Error(err))

	dstCloseCtx := misc.WithNoCancelIn(ctx, nbsTimeout)

	err1 := mc.dst.Close(dstCloseCtx, err)
	if err1 != nil && err == nil {
		err = err1
	}

	srcCloseCtx := misc.WithNoCancelIn(ctx, nbsTimeout)

	err1 = mc.src.Close(srcCloseCtx, err)
	if err1 != nil && err == nil {
		err = err1
	}

	if mc.request.Params.HeartbeatEnabled {
		// If heartbeat elapsed, we will have wrong err.
		err = mc.stopHeartbeat(ctx, err)
	}
	mc.finish(ctx, err)
	return err
}

func (mc *MoveContext) copy(ctx context.Context, workerID int) (resErr error) {
	cfg, _ := config.GetConfig()

	traceWorker := cfg.Tracing.TraceAllCopyWorkers || workerID == 0

	if traceWorker {
		var span opentracing.Span
		span, ctx = tracing.StartSpanFromContextFunc(ctx)
		defer func() { tracing.Finish(span, resErr) }()

		span.LogFields(otlog.String("message", "next spans will be sampled"))
	} else {
		_, ctx = tracing.DisableSpans(ctx)
	}

	var loopSpan opentracing.Span
	defer func() {
		if loopSpan != nil {
			tracing.Finish(loopSpan, resErr)
		}
	}()

	tracingLimiter := tracing.SpanRateLimiter{
		InitialInterval: cfg.Tracing.LoopRateLimiterInitialInterval.Duration,
		MaxInterval:     cfg.Tracing.LoopRateLimiterMaxInterval.Duration,
	}

	for {
		if loopSpan != nil {
			tracing.Finish(loopSpan, resErr)
		}

		var loopCtx context.Context
		loopSpan, loopCtx = tracingLimiter.StartSpanFromContext(ctx, "copy-chunk")

		// read
		mc.mutex.Lock()
		offset := mc.nextOffset
		mc.nextOffset += mc.blockSize
		if offset > mc.size {
			offset = mc.size
		}
		mc.currentOffset[workerID] = offset
		mc.mutex.Unlock()

		if offset >= mc.size {
			return nil
		}

		select {
		case <-loopCtx.Done():
			return loopCtx.Err()
		default:
		}

		data := mc.pool.Get().(*[]byte)
		exists, zero, err := mc.src.ReadAt(loopCtx, offset, *data)
		if err != nil {
			logLevel := zapcore.ErrorLevel
			if err == io.EOF {
				logLevel = zapcore.InfoLevel
			}
			log.LevelCtx(loopCtx, logLevel, "Error while read data during move", zap.Error(err))
			mc.pool.Put(data)
			return err
		}

		if !exists && mc.request.Params.SkipNonexistent || zero && mc.request.Params.SkipZeroes {
			loopSpan.LogFields(otlog.Message("skip write"))
			mc.pool.Put(data)
			continue
		}

		select {
		case <-loopCtx.Done():
			mc.pool.Put(data)
			return loopCtx.Err()
		default:
		}

		// write
		if zero {
			err = mc.dst.WriteZeroesAt(loopCtx, offset, int(mc.blockSize))
		} else {
			err = mc.dst.WriteAt(loopCtx, offset, *data)
		}
		mc.pool.Put(data)
		if err != nil {
			return err
		}
	}
}

func (mc *MoveContext) getBlockSize(src, dst move.BlockDevice) int64 {
	srcBlockSize := src.GetBlockSize()
	dstBlockSize := dst.GetBlockSize()

	blockSize := srcBlockSize * dstBlockSize /
		misc.GcdInt64(srcBlockSize, dstBlockSize)
	return blockSize
}

func (mc *MoveContext) init(ctx context.Context) (err error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, err) }()

	log.G(ctx).Debug("Init move context", zap.Any("request", mc.request))

	mc.src, err = mc.mcf.mdf.GetSrc(ctx, mc.request.Src, mc.request.Params)

	if err != nil {
		return err
	}

	switch t := mc.request.Src.(type) {
	case *common.URLMoveSrc:
		c := metriclabels.WithFormat(ctx, t.Format)
		mc.timer = misc.URLMoveTaskTimer.Start(c)
		mc.observer = misc.URLMoveTaskSpeed.Observer(c)
	case *common.NbsMoveSrc:
		mc.timer = misc.NBSMoveTaskTimer.Start(ctx)
		mc.observer = misc.NBSMoveTaskSpeed.Observer(ctx)
	}

	if verifier, ok := mc.src.(move.ConsistencyVerifier); ok && mc.request.Params.Offset == 0 {
		// Invalid snapshot found
		if err := verifier.VerifyMetadata(ctx); err != nil {
			return err
		}
	}

	hint := mc.src.GetHint()

	mc.dst, err = mc.mcf.mdf.GetDst(ctx, mc.request.Dst, hint)
	if err != nil {
		ctx = log.WithLogger(context.Background(), log.G(ctx))
		ctx, cancelSRC := context.WithTimeout(ctx, nbsTimeout)
		defer cancelSRC()
		_ = mc.src.Close(ctx, err)
		return err
	}

	// Calculate workers count
	switch {
	case mc.request.Params.WorkersCount != 0:
		mc.workersCount = mc.request.Params.WorkersCount
	case hint.ReaderCount != 0:
		mc.workersCount = hint.ReaderCount
	default:
		mc.workersCount = mc.mcf.workersCount
	}

	// Calculate operation block size
	blockSize := mc.getBlockSize(mc.src, mc.dst)
	if mc.request.Params.BlockSize != 0 {
		if mc.request.Params.BlockSize%blockSize != 0 {
			err = misc.ErrInvalidBlockSize
			ctx = log.WithLogger(context.Background(), log.G(ctx))
			log.G(ctx).Error("MoveContext.Init failed", zap.Error(err))
			srcCloseCtx, cancelSRC := context.WithTimeout(ctx, nbsTimeout)
			defer cancelSRC()
			_ = mc.src.Close(srcCloseCtx, err)
			dstCloseCtx, cancelDST := context.WithTimeout(ctx, nbsTimeout)
			defer cancelDST()
			_ = mc.dst.Close(dstCloseCtx, err)
			return err
		}
		blockSize = mc.request.Params.BlockSize
	}
	mc.blockSize = blockSize

	// Calculate copy size
	if mc.request.Params.SkipZeroes {
		mc.size = mc.src.GetSize()
	} else {
		mc.size = mc.dst.GetSize()
	}

	mc.pool = sync.Pool{
		New: func() interface{} {
			b := make([]byte, blockSize)
			return &b
		},
	}
	mc.nextOffset = mc.request.Params.Offset
	mc.currentOffset = make(map[int]int64)
	mc.flushedOffset = mc.request.Params.Offset

	return nil
}

func (mc *MoveContext) finish(ctx context.Context, err error) {
	mc.mutex.Lock()
	mc.finished = true
	mc.err = err
	t := time.Now()
	mc.finishedAt = &t
	mc.mutex.Unlock()
	log.G(ctx).Debug("MoveContext finished", zap.Error(err))

	var dur time.Duration
	if mc.timer != nil {
		dur = mc.timer.ObserveDuration()
	}

	if mc.observer != nil {
		mc.observer.Observe(misc.Speed64(mc.size, dur))
	}
}
