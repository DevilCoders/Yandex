package kmslbclient

import (
	"sync"
	"time"
)

type fixedTimeWindow struct {
	lock  sync.Mutex
	times []time.Time
	start uint64
	end   uint64
}

func newFixedTimeWindow(size int) *fixedTimeWindow {
	return &fixedTimeWindow{
		times: make([]time.Time, size),
	}
}

func (w *fixedTimeWindow) add(t time.Time) (time.Duration, bool) {
	l := uint64(len(w.times))
	w.lock.Lock()
	// NOTE: We do not guarantee the monotonicity of times slice. We could by getting time.Now() by taking the lock, but that would
	// limit the scalability and serve little purpose (if any).
	w.times[w.end%l] = t
	w.end++
	if w.end-w.start > l {
		w.start++
	}
	diff := t.Sub(w.times[w.start%l])
	full := w.end-w.start == l
	w.lock.Unlock()
	return diff, full
}

func (w *fixedTimeWindow) diff() time.Duration {
	l := uint64(len(w.times))
	w.lock.Lock()
	var first, last time.Time
	if w.end > 0 {
		first = w.times[w.start%l]
		last = w.times[(w.end-1)%l]
	}
	w.lock.Unlock()
	return last.Sub(first)
}

func (w *fixedTimeWindow) count() int {
	w.lock.Lock()
	ret := w.end - w.start
	w.lock.Unlock()
	return int(ret)
}
