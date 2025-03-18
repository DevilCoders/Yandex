package uazap

import (
	"errors"
	"fmt"
	"sync"
	"sync/atomic"
	"time"

	uaclient "a.yandex-team.ru/library/go/yandex/uagent/log/zap/client"
)

// background is a single object shared by all clones of core.
type background struct {
	options options

	q          queue
	client     uaclient.Client
	clientStat uaclient.Stats

	wg      sync.WaitGroup
	mu      sync.Mutex
	cond    *sync.Cond
	stopped bool

	iterRunning bool
	iter        int64

	lastErr    error
	forceFlush chan struct{}

	stat           Stat
	reportOverflow int64
}

func newBackground(options options, client uaclient.Client) *background {
	b := &background{
		options:    options,
		client:     client,
		clientStat: client.Stat(),
		forceFlush: make(chan struct{}, 1),
	}
	b.cond = sync.NewCond(&b.mu)
	b.wg.Add(1)
	go b.run()

	return b
}

func (b *background) flush(messages []uaclient.Message) {
	if err := b.client.Send(messages); err != nil {
		b.stat.dropInflightMessages(messages)
		b.lastErr = err
	}
}

func (b *background) stop() {
	b.mu.Lock()
	b.stopped = true
	b.mu.Unlock()

	// Need to do force flush to avoid possible waiting for another tick.
	select {
	case b.forceFlush <- struct{}{}:
	default:
	}

	b.wg.Wait()
}

func (b *background) finishIter() (stop bool) {
	// isStopped check must be before broadcast. Otherwise following situation may occur:
	// 1) Broadcast here
	// 2) New sync appears and moves to cond.Wait()
	// 3) Core closes, finishIter returns true and sync from step 2 waits forever.
	b.iter++
	b.iterRunning = false

	atomic.StoreInt64(&b.reportOverflow, 0)
	b.cond.Broadcast()
	return
}

func (b *background) run() {
	defer b.wg.Done()

	flush := time.NewTicker(b.options.flushInterval)
	defer flush.Stop()

	for {
		b.mu.Lock()
		b.iterRunning = true

		messages := b.q.dequeueAll()
		if b.stopped {
			b.finishIter()
			b.mu.Unlock()

			for _, m := range messages {
				b.stat.dropMessage(m.Size())
			}
			queuePool.Put(messages)
			return
		}
		b.mu.Unlock()

		if len(messages) != 0 {
			b.flush(messages)
		}
		queuePool.Put(messages)

		b.mu.Lock()
		b.finishIter()
		b.mu.Unlock()

		select {
		case <-flush.C:
		case <-b.forceFlush:
			flush.Reset(b.options.flushInterval)
		}
	}
}

func (b *background) emplace(message uaclient.Message) error {
	messageSize := message.Size()

	b.stat.receiveMessage(messageSize)

	b.mu.Lock()
	defer b.mu.Unlock()

	if b.stopped {
		b.stat.dropMessage(messageSize)
		if old := atomic.SwapInt64(&b.reportOverflow, 1); old == 1 {
			return nil
		}
		return errors.New("core has stopped")
	}

	if messageSize > b.client.GRPCMaxMessageSize() {
		b.stat.dropMessage(messageSize)
		if old := atomic.SwapInt64(&b.reportOverflow, 1); old == 1 {
			return nil
		}
		return fmt.Errorf("log message size exceeded: %d > %d", messageSize, b.client.GRPCMaxMessageSize())
	}
	if !b.options.rateLimiter.AllowN(time.Now(), int(messageSize)) {
		b.stat.dropMessage(messageSize)

		if old := atomic.SwapInt64(&b.reportOverflow, 1); old == 1 {
			return nil
		}
		return fmt.Errorf("log message with size %d is ratelimited", messageSize)
	}

	if newSize, ok, shouldReport := b.checkOverflow(messageSize); !ok {
		// Report overflow error only once per background iteration, to avoid spamming error output.
		if !shouldReport {
			return nil
		}

		return fmt.Errorf("logger queue overflow: %d >= %d", newSize, b.options.maxMemoryUsage)
	}

	b.q.enqueue(message)
	return nil
}

func (b *background) checkOverflow(messageSize int64) (size int, ok, shouldReport bool) {
	b.stat.mu.Lock()
	defer b.stat.mu.Unlock()

	if size = int(b.stat.InflightBytes + messageSize - b.clientStat.AckedBytes()); size >= b.options.maxMemoryUsage {
		b.stat.dropMessageLocked(messageSize)

		old := atomic.SwapInt64(&b.reportOverflow, 1)
		return size, false, old == 0
	}

	b.stat.inflightMessageLocked(messageSize)
	return 0, true, false
}

func (b *background) sync() error {
	b.mu.Lock()
	defer b.mu.Unlock()

	select {
	case b.forceFlush <- struct{}{}:
	default:
	}

	flushIter := b.iter + 1
	if b.iterRunning {
		flushIter++
	}

	for {
		if b.iter >= flushIter {
			return b.lastErr
		}

		if b.stopped {
			return errors.New("core has stopped")
		}

		b.cond.Wait()
	}
}
