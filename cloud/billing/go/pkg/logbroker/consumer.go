package logbroker

import (
	"context"
	"errors"
	"fmt"
	"sync"
	"time"

	"a.yandex-team.ru/cloud/billing/go/pkg/errsentinel"
	"a.yandex-team.ru/cloud/billing/go/pkg/logbroker/lbtypes"
	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue"
	"a.yandex-team.ru/library/go/core/log"
)

const idleCyclesToShutdown = 20 // approx 5 min

// consumer stores reader and encapsulates logbroker messages processing logic.
// whole instance of this type shod be thrown if reader fails
type consumer struct {
	ctx    context.Context // this one will be canceled when we want to stop consumer
	logger log.Logger

	reader persqueue.Reader // constructs in service externally

	// this params control of data accumulation in consecutive logbroker reads
	readDuration time.Duration
	readLimit    int
	sizeLimit    int

	// params from service to control message handling and offsets locking
	handler lbtypes.Handler
	offsets lbtypes.OffsetReporter

	// infly batches storage - another abstraction layer
	inflyMessages

	counters *counters

	idleCycles int
}

// run starts consuption loop and controls cancellations
func (c *consumer) run() (err error) {
	// This context controls reading session.
	// If it canceled then all processing and reading will be terminated.
	runCtx, cancel := context.WithCancel(context.Background())
	defer cancel()

	init, startErr := c.reader.Start(runCtx)
	if startErr != nil {
		return errFatal.Wrap(startErr)
	}

	c.logger = log.With(c.logger, log.String("lb_session_id", init.SessionID))
	c.logger.Debug("reader started")

	go func() { // process logbroker events
		err = c.loop(runCtx)
		cancel()
	}()

	go func() { // wait if reader abnormally terminated
		<-c.reader.Closed()
		cancel()
	}()

	select {
	case <-c.ctx.Done(): // external reader shutdown request (service stop or suspend)
	case <-runCtx.Done(): // loop exited with error and reader destroyed by context cancellation
		return
	}

	c.reader.Shutdown()

	// logbroker can hangs during shutdown
	timeout := time.NewTimer(time.Second * 10)
	defer timeout.Stop()
	select {
	case <-runCtx.Done():
	case <-timeout.C:
		cancel()
		return errors.New("logbroker reader termination timeout")
	}

	return
}

// loop through incoming messages
func (c *consumer) loop(ctx context.Context) error {
	// timer to periodically force to process incoming data
	timer := time.NewTimer(c.readDuration)
	defer timer.Stop()
	defer c.resetInfly()
	var callback func() error

	for {
		timeToGo := false // is it time to force process?
		callback = nil
		select {
		case e, ok := <-c.reader.C():
			if !ok { // reader shuting down
				return c.reader.Err()
			}
			c.counters.gotEvent(time.Now())
			forceCallback, err := c.handleEvent(ctx, e)
			if err != nil {
				return err
			}
			timeToGo = forceCallback != nil
			callback = forceCallback
		case <-timer.C:
			timeToGo = true
		case <-ctx.Done(): // force exit because of external stop
			return nil
		}

		// try to process collected data, if amount too small then processed = false
		// and we continue to accumulate more messages
		processed, err := c.runProcessing(timeToGo)
		if err != nil {
			return err
		}
		if callback != nil {
			if err := callback(); err != nil {
				return err
			}
		}
		if processed {
			if c.idleCycles > idleCyclesToShutdown {
				return nil
			}
			timer.Reset(c.readDuration)
		}
	}
}

// runProcessing checks conditions and run messages handling
func (c *consumer) runProcessing(force bool) (bool, error) {
	// check if we should handle
	needProcess := force
	if !needProcess {
		needProcess = c.checkLimit(c.readLimit, c.sizeLimit)
	}
	if !needProcess {
		return false, nil
	}

	// get messages to handle from inflights storage
	params, commiters := c.getHandlingParams()
	if len(params) == 0 { // if no messages read
		c.idleCycles++
		return true, nil
	}
	c.idleCycles = 0

	// run messages handling concurrently by source
	wg := sync.WaitGroup{}
	wg.Add(len(params))
	errors := make([]error, len(params))
	for i := range params {
		param := params[i]
		reporter := handlerReporter{
			errs:   errors,
			idx:    i,
			logger: log.With(c.logger, log.String("lb_source", param.src.String())),
		}
		go func() {
			defer wg.Done()
			c.counters.handleStart()

			c.handleSource(param.ctx, param.sourceMessages, reporter)

			reporter.Error(param.ctx.Err())
			switch {
			case param.ctx.Err() != nil:
				c.counters.handleCancel()
			case reporter.err() != nil:
				c.counters.handleFailed()
			default:
				c.counters.handleDone()
			}
		}()
	}
	wg.Wait()
	c.counters.inflyUpdated(0, 0)

	// NOTE: all retries and error handling logic should be done in handler.
	// if we get error here - there are unretriable error that cause to restart reader
	for _, err := range errors {
		if err != nil {
			return true, errProcessing
		}
	}

	for _, c := range commiters {
		c.Commit()
	}
	return true, nil
}

// handleSource is handle all accumulated messages of one source
func (c *consumer) handleSource(ctx context.Context, sm sourceMessages, reporter handlerReporter) {
	defer reporter.log()
	defer func() {
		if r := recover(); r != nil {
			reporter.Error(fmt.Errorf("handler panics: %v", r))
		}
	}()

	ctx, cancel := context.WithCancel(ctx)
	defer cancel()

	srcID := lbtypes.SourceID(sm.src.String())

	lastID := len(sm.messages) - 1
	messages := lbtypes.Messages{
		MessagesReporter: reporter,
		Messages:         castMessages(sm.messages),
		LastOffset:       sm.messages[lastID].Offset,
		LastWriteTime:    sm.messages[lastID].WriteTime,
	}

	// Release data from sdk struct
	for i := range sm.messages {
		sm.messages[i].Data = nil
	}

	c.handler.Handle(ctx, srcID, &messages)
}

var (
	errFatal      = errsentinel.New("logbroker fatal error")
	errProcessing = errors.New("messages processing failed")
)

// handlerReporter sets error in slice of errors by callback call from handler
type handlerReporter struct {
	errs   []error
	idx    int
	logger log.Logger
}

func (r handlerReporter) Consumed() {
	r.errs[r.idx] = nil
}

func (r handlerReporter) Error(err error) {
	if r.errs[r.idx] != nil || err == nil {
		return
	}
	r.errs[r.idx] = err
}

func (r handlerReporter) log() {
	if r.errs[r.idx] != nil {
		r.logger.Error("messages processing failed", log.Error(r.errs[r.idx]))
		return
	}
	r.logger.Debug("messages successfully processed")
}

func (r handlerReporter) err() error {
	return r.errs[r.idx]
}
