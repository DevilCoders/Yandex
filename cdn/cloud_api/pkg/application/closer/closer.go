package closer

import (
	"os"
	"os/signal"
	"sync"
	"time"

	"a.yandex-team.ru/library/go/core/log"
)

type Closer struct {
	mutex sync.Mutex
	once  sync.Once
	done  chan struct{}

	closeTimeout time.Duration
	logger       log.Logger
	funcs        []func() error
}

func NewCloser(logger log.Logger, closeTimeout time.Duration, sig ...os.Signal) *Closer {
	c := &Closer{
		mutex:        sync.Mutex{},
		once:         sync.Once{},
		done:         make(chan struct{}),
		closeTimeout: closeTimeout,
		logger:       logger,
		funcs:        nil,
	}

	if len(sig) > 0 {
		go func() {
			ch := make(chan os.Signal, 1)
			signal.Notify(ch, sig...)
			<-ch
			signal.Stop(ch)
			c.CloserAll()
		}()
	}

	return c
}

func (c *Closer) Run(f func() error) {
	go func() {
		err := f()
		if err != nil {
			c.logger.Error("error returned from closer task", log.Error(err))
			c.CloserAll()
		}
	}()
}

func (c *Closer) Add(f ...func() error) {
	c.mutex.Lock()
	c.funcs = append(c.funcs, f...)
	c.mutex.Unlock()
}

func (c *Closer) Wait() {
	<-c.done
}

// TODO: test, close channel?
func (c *Closer) CloserAll() {
	c.once.Do(func() {
		defer close(c.done)

		c.mutex.Lock()
		funcs := c.funcs
		c.mutex.Unlock()

		errors := make(chan error, len(funcs))
		wg := &sync.WaitGroup{}
		for _, f := range funcs {
			wg.Add(1)
			go func(f func() error) {
				errors <- f()
				wg.Done()
			}(f)
		}

		if ok := waitWithTimeout(wg, c.closeTimeout); !ok {
			c.logger.Error("closer timeout expired")
		}

		for {
			select {
			case err := <-errors:
				if err != nil {
					c.logger.Error("error returned from closer", log.Error(err))
				}
			default:
				return
			}
		}
	})
}

func waitWithTimeout(wg *sync.WaitGroup, timeout time.Duration) (ok bool) {
	ch := make(chan struct{})
	go func() {
		wg.Wait()
		close(ch)
	}()

	timer := time.NewTimer(timeout)
	defer timer.Stop()

	select {
	case <-ch:
		return true
	case <-timer.C:
		return false
	}
}
