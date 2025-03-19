package logbroker

import (
	"container/list"
	"context"
	"sync"
)

type waiter struct {
	n     int
	ready chan<- struct{} // Closed when semaphore acquired.
}

// limiter is adaptation of golang.org/x/sync/semaphore.Weighted for overcommits
type limiter struct {
	size    int
	cur     int
	mu      sync.Mutex
	waiters list.List
}

func (s *limiter) SetSize(n int) {
	s.mu.Lock()
	s.size = n
	s.notifyWaiters()
	s.mu.Unlock()
}

func (s *limiter) Acquire(ctx context.Context, n int) error {
	if s.size <= 0 {
		return nil
	}
	s.mu.Lock()
	if s.size > s.cur && s.waiters.Len() == 0 {
		s.cur += n
		s.mu.Unlock()
		return nil
	}

	ready := make(chan struct{})
	w := waiter{n: n, ready: ready}
	elem := s.waiters.PushBack(w)
	s.mu.Unlock()

	select {
	case <-ctx.Done():
		err := ctx.Err()
		s.mu.Lock()
		select {
		case <-ready:
			err = nil
		default:
			s.waiters.Remove(elem)
		}
		s.mu.Unlock()
		return err

	case <-ready:
		return nil
	}
}

// Release releases the semaphore with a weight of n.
func (s *limiter) Release(n int) {
	if s.size <= 0 {
		return
	}
	s.mu.Lock()
	s.cur -= n
	if s.cur < 0 {
		s.mu.Unlock()
		s.cur = 0
	}
	s.notifyWaiters()
	s.mu.Unlock()
}

func (s *limiter) notifyWaiters() {
	for {
		next := s.waiters.Front()
		if next == nil {
			break // No more waiters blocked.
		}

		w := next.Value.(waiter)
		if s.size <= s.cur {
			break
		}

		s.cur += w.n
		s.waiters.Remove(next)
		close(w.ready)
	}
}
