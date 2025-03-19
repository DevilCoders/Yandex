package logbroker

import (
	"context"
	"errors"
	"math/rand"
	"sync"
	"time"

	"a.yandex-team.ru/cloud/billing/go/pkg/logbroker/lbtypes"
	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue"
	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue/log/corelogadapter"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
)

// ConsumerService is long living object which handle logbroker messages with take care of errors, reconnections,
// batch sizes and other common boilerplate.
type ConsumerService struct {
	cfg consumerConfig

	handler lbtypes.Handler
	offsets lbtypes.OffsetReporter

	logger log.Logger

	muState sync.RWMutex

	status   lbtypes.ServiceStatus
	counters counters

	runCtx context.Context
	stop   context.CancelFunc

	stopped chan struct{}

	suspend     context.CancelFunc
	resumeTimer *time.Timer
	waitResume  chan struct{}

	queueOverride       func(persqueue.ReaderOptions) persqueue.Reader // For tests
	suspendTimeOverride func() time.Duration
}

// NewConsumerService constructs service by options. Required params:
//  - handler to process incoming data, should allow to use it concurrently.
//  - offsets to report queue offsets by source names. If no special handling needed use ReadCommittedOffsets.
//
func NewConsumerService(
	handler lbtypes.Handler, offsets lbtypes.OffsetReporter, options ...ConsumerOption,
) (*ConsumerService, error) {
	if handler == nil {
		return nil, errors.New("handler should not be nil")
	}
	if offsets == nil {
		return nil, errors.New("offsets getter should not be nil")
	}

	result := ConsumerService{
		handler: handler,
		offsets: offsets,
		logger:  nopLogger,
		status:  lbtypes.ServiceNotRunning,
		stopped: make(chan struct{}),
	}
	result.runCtx, result.stop = context.WithCancel(context.Background())

	defaultConfig(&result.cfg)
	for _, opt := range options {
		opt(&result.cfg)
	}

	if result.cfg.Logger != nil {
		result.logger = result.cfg.Logger
	}

	if result.cfg.ReaderOptions.Logger == nil {
		result.cfg.ReaderOptions.Logger = corelogadapter.New(result.cfg.Logger)
	}

	return &result, nil
}

// Run service. This method will block until Stop called or fatal error occur.
func (c *ConsumerService) Run() error {
	if err := c.prepareRun(); err != nil {
		return err
	}
	defer c.runDone()

	for {
		var ctx context.Context
		var err error
		if ctx, err = c.prepareConsume(); err != nil {
			return err
		}
		if err := c.consumeLoop(ctx); err != nil {
			return ErrConsumerFailed.Wrap(err)
		}

		c.waitSuspended()
		if c.runCtx.Err() != nil {
			break
		}
	}
	return nil
}

// Suspend cause service to pause. If d > 0 service will be resumed automatically.
func (c *ConsumerService) Suspend(d time.Duration) {
	defer lock(&c.muState).Unlock()

	if c.suspend == nil { // not running
		return
	}

	c.suspend()
	c.suspend = nil

	if c.status != lbtypes.ServiceError {
		c.status = lbtypes.ServiceSuspending
	}
	c.waitResume = make(chan struct{})

	if d <= 0 {
		return
	}

	c.resumeTimer = time.AfterFunc(d, func() {
		_ = c.Resume()
	})
}

// Resume suspended service.
func (c *ConsumerService) Resume() error {
	defer lock(&c.muState).Unlock()
	if c.waitResume == nil {
		return nil
	}

	if c.resumeTimer != nil {
		c.resumeTimer.Stop()
		c.resumeTimer = nil
	}
	close(c.waitResume)
	c.waitResume = nil

	c.status = lbtypes.ServiceStarting

	return nil
}

// Stop will gracefully stops service. In progress reads will be processed and committed.
func (c *ConsumerService) Stop(ctx context.Context) error {
	c.muState.Lock()

	if c.stop == nil {
		c.muState.Unlock()
		return nil
	}

	c.stop()
	stopped := c.stopped
	c.status = lbtypes.ServiceShutdown
	c.muState.Unlock()

	select {
	case <-stopped:
		return nil
	case <-ctx.Done():
		return ErrStillRunning
	}
}

// Stats returns service status and counters.
func (c *ConsumerService) Stats() lbtypes.ServiceCounters {
	defer rlock(&c.muState).Unlock()
	cv, pq := c.counters.getValues()

	return lbtypes.ServiceCounters{
		Status:              c.status,
		InflyMessages:       cv.inflies,
		ReadMessages:        cv.readMessages,
		HandleCalls:         cv.handleCalls,
		Handled:             cv.handled,
		HandleFailures:      cv.handleFailures,
		HandleCancellations: cv.handleCancellations,
		Locks:               cv.locks,
		ActiveLocks:         cv.activeLocks,
		LastEventAt:         cv.lastEventAt,
		Restarts:            cv.restarts,
		Suspends:            cv.suspends,
		ReaderStats: lbtypes.Stat{
			MemUsage:       pq.MemUsage,
			InflightCount:  pq.InflightCount,
			WaitAckCount:   pq.WaitAckCount,
			BytesExtracted: pq.BytesExtracted,
			BytesRead:      pq.BytesRead,
			SessionID:      pq.SessionID,
		},
	}
}

var nopLogger = &nop.Logger{}

func (c *ConsumerService) prepareRun() error {
	defer lock(&c.muState).Unlock()

	c.counters = counters{}
	c.counters.init()

	c.status = lbtypes.ServiceStarting
	return nil
}

func (c *ConsumerService) runDone() {
	defer lock(&c.muState).Unlock()

	if c.suspend != nil {
		c.suspend()
		c.suspend = nil
	}
	if c.waitResume != nil {
		close(c.waitResume)
		c.waitResume = nil
	}
	c.stop()
	c.counters.shutdown()

	if c.status != lbtypes.ServiceError {
		c.status = lbtypes.ServiceNotRunning
	}

	close(c.stopped)
}

func (c *ConsumerService) prepareConsume() (ctx context.Context, err error) {
	defer lock(&c.muState).Unlock()

	if c.suspend != nil {
		c.suspend()
	}

	ctx, c.suspend = context.WithCancel(c.runCtx)
	return
}

func (c *ConsumerService) consumeLoop(ctx context.Context) error {
	queueConstructor := persqueue.NewReader
	if c.queueOverride != nil {
		queueConstructor = c.queueOverride
	}

	reader := queueConstructor(c.cfg.ReaderOptions)
	readDuration := c.cfg.maxReadDuration
	if readDuration == 0 {
		readDuration = time.Second * 15
	}

	cons := consumer{
		ctx:    ctx,
		logger: c.logger, // Maybe add name

		reader: reader,

		readDuration: readDuration,
		readLimit:    c.cfg.readLimit,
		sizeLimit:    c.cfg.sizeLimit,

		handler:  c.handler,
		offsets:  c.offsets,
		counters: &c.counters,
	}

	cons.resetInfly()

	c.counters.setPQ(cons.reader)
	defer c.counters.resetPQ()

	c.lockSetStatus(lbtypes.ServiceRunning)
	err := cons.run()
	c.counters.readerReseted()

	if errors.Is(err, errFatal) {
		c.lockSetStatus(lbtypes.ServiceError)
		c.logger.Error("logbroker fatal error", log.Error(err))
		if !c.cfg.skipFatals {
			return err
		}
	}

	if ctx.Err() != nil { // shutdown
		return nil
	}

	if err != nil && !c.cfg.skipBackPressure { // error should be already logged and we check if should get some sleep
		var suspendFor time.Duration
		if c.suspendTimeOverride != nil {
			suspendFor = c.suspendTimeOverride()
		} else {
			suspendFor = randomDelay(autoSuspendDuration, autoSuspendJitter)
		}
		c.Suspend(suspendFor)
	}

	return nil
}

func (c *ConsumerService) waitSuspended() {
	c.muState.Lock()
	defer c.counters.restarted()

	if c.waitResume == nil {
		c.muState.Unlock()
		return
	}

	if c.status != lbtypes.ServiceError {
		c.status = lbtypes.ServiceSuspended
	}
	c.counters.suspended()

	wait := c.waitResume
	c.muState.Unlock()

	select {
	case <-wait:
	case <-c.runCtx.Done():
	}
}

func (c *ConsumerService) lockSetStatus(s lbtypes.ServiceStatus) {
	defer lock(&c.muState).Unlock()
	c.status = s
}

const (
	autoSuspendDuration = time.Minute
	autoSuspendJitter   = time.Second * 30
)

func randomDelay(duration time.Duration, jitter time.Duration) time.Duration {
	return (duration - jitter) + time.Duration(rand.Int63n(int64(jitter*2)))
}
