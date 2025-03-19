package logbroker

import (
	"context"
	"sync"
	"time"

	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue"
)

type countersValues struct {
	restarts int
	suspends int

	handleCalls         int
	handled             int
	handleFailures      int
	handleCancellations int

	inflies      int
	readMessages int

	locks       int
	activeLocks int

	lastEventAt time.Time
}

type pqStats interface {
	Stat() persqueue.Stat
}

type counters struct {
	mu sync.RWMutex

	runCtx  context.Context
	stop    context.CancelFunc
	changes chan counterApplier
	alive   chan struct{}

	initOnce     sync.Once
	shutdownOnce sync.Once

	persqueueStats pqStats
	countersValues
}

func (c *counters) init() {
	c.initOnce.Do(func() {
		c.runCtx, c.stop = context.WithCancel(context.Background())
		c.changes = make(chan counterApplier, 1000) // to not lock on this channel
		c.alive = make(chan struct{})
		go c.applier()
	})
}

func (c *counters) shutdown() {
	c.shutdownOnce.Do(func() {
		c.stop()
		close(c.changes)
	})
	<-c.alive
}

func (c *counters) setPQ(s pqStats) {
	defer lock(&c.mu).Unlock()
	c.persqueueStats = s
}

func (c *counters) resetPQ() {
	defer lock(&c.mu).Unlock()
	c.persqueueStats = nil
}

func (c *counters) getValues() (countersValues, persqueue.Stat) {
	defer rlock(&c.mu).Unlock()

	var pq persqueue.Stat
	if c.persqueueStats != nil {
		pq = c.persqueueStats.Stat()
	}

	return c.countersValues, pq
}

func (c *counters) restarted() {
	defer skipPanic()
	select {
	case <-c.runCtx.Done():
	case c.changes <- applyFunc(incRestarts):
	}
}

func (c *counters) suspended() {
	defer skipPanic()
	select {
	case <-c.runCtx.Done():
	case c.changes <- applyFunc(incSuspends):
	}
}

func (c *counters) readerReseted() {
	defer skipPanic()
	select {
	case <-c.runCtx.Done():
	case c.changes <- applyFunc(resetReader):
	}
}

func (c *counters) handleStart() {
	defer skipPanic()
	select {
	case <-c.runCtx.Done():
	case c.changes <- applyFunc(incHandleCalls):
	}
}

func (c *counters) handleDone() {
	defer skipPanic()
	select {
	case <-c.runCtx.Done():
	case c.changes <- applyFunc(incHandled):
	}
}

func (c *counters) handleFailed() {
	defer skipPanic()
	select {
	case <-c.runCtx.Done():
	case c.changes <- applyFunc(incHandleFailures):
	}
}

func (c *counters) handleCancel() {
	defer skipPanic()
	select {
	case <-c.runCtx.Done():
	case c.changes <- applyFunc(incHandleCancellations):
	}
}

func (c *counters) inflyUpdated(count int, inc int) {
	defer skipPanic()
	select {
	case <-c.runCtx.Done():
	case c.changes <- messagesSetter{infly: count, readInc: inc}:
	}
}

func (c *counters) lockUpdated(active int, locked int) {
	defer skipPanic()
	select {
	case <-c.runCtx.Done():
	case c.changes <- locksSetter{active: active, locksInc: locked}:
	}
}

func (c *counters) gotEvent(t time.Time) {
	defer skipPanic()
	select {
	case <-c.runCtx.Done():
	case c.changes <- eventSetter{at: t}:
	}
}

// skipPanic skips some panics in stats apply because they are sometimes happens on shutdown and are not important
func skipPanic() {
	_ = recover()
}

// Async appling implementation

func (c *counters) applier() {
	changeChan := c.changes
	for ch := range changeChan {
		ch.apply(c)
	}
	close(c.alive)
}

type counterApplier interface {
	apply(c *counters)
}

type applyFunc func(*counters)

func (f applyFunc) apply(c *counters) {
	f(c)
}

func resetReader(c *counters) {
	defer lock(&c.mu).Unlock()
	c.inflies = 0
	c.activeLocks = 0
}

func incRestarts(c *counters) {
	defer lock(&c.mu).Unlock()
	c.restarts++
}

func incSuspends(c *counters) {
	defer lock(&c.mu).Unlock()
	c.suspends++
}

func incHandleCalls(c *counters) {
	defer lock(&c.mu).Unlock()
	c.handleCalls++
}

func incHandled(c *counters) {
	defer lock(&c.mu).Unlock()
	c.handled++
}

func incHandleFailures(c *counters) {
	defer lock(&c.mu).Unlock()
	c.handleFailures++
}

func incHandleCancellations(c *counters) {
	defer lock(&c.mu).Unlock()
	c.handleCancellations++
}

type messagesSetter struct {
	infly   int
	readInc int
}

func (i messagesSetter) apply(c *counters) {
	defer lock(&c.mu).Unlock()
	c.inflies = i.infly
	c.readMessages += i.readInc
}

type locksSetter struct {
	active   int
	locksInc int
}

func (l locksSetter) apply(c *counters) {
	defer lock(&c.mu).Unlock()
	c.activeLocks = l.active
	c.locks += l.locksInc
}

type eventSetter struct {
	at time.Time
}

func (e eventSetter) apply(c *counters) {
	defer lock(&c.mu).Unlock()
	if c.lastEventAt.Before(e.at) {
		c.lastEventAt = e.at
	}
}
