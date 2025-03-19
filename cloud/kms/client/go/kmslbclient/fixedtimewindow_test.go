package kmslbclient

import (
	"sync"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
)

var (
	zeroDuration time.Duration
)

func TestFixedTimeWindow1(t *testing.T) {
	w := newFixedTimeWindow(1)
	assert.EqualValues(t, w.count(), 0)
	assert.EqualValues(t, w.diff(), zeroDuration)

	dt, full := w.add(time.Unix(123456, 0))
	assert.True(t, full)
	assert.EqualValues(t, 1, w.count())
	assert.EqualValues(t, zeroDuration, w.diff())
	assert.EqualValues(t, zeroDuration, dt)

	dt, full = w.add(time.Unix(123457, 0))
	assert.True(t, full)
	assert.EqualValues(t, 1, w.count())
	assert.EqualValues(t, zeroDuration, w.diff())
	assert.EqualValues(t, zeroDuration, dt)
}

func TestFixedTimeWindow2(t *testing.T) {
	w := newFixedTimeWindow(2)
	assert.EqualValues(t, w.count(), 0)
	assert.EqualValues(t, w.diff(), zeroDuration)

	time1 := time.Unix(123456, 0)
	dt, full := w.add(time1)
	assert.False(t, full)
	assert.EqualValues(t, 1, w.count())
	assert.EqualValues(t, zeroDuration, w.diff())
	assert.EqualValues(t, zeroDuration, dt)

	time2 := time1.Add(time.Second)
	dt, full = w.add(time2)
	assert.True(t, full)
	assert.EqualValues(t, 2, w.count())
	assert.EqualValues(t, time2.Sub(time1), w.diff())
	assert.EqualValues(t, time2.Sub(time1), dt)

	time3 := time2.Add(3 * time.Second)
	dt, full = w.add(time3)
	assert.True(t, full)
	assert.EqualValues(t, 2, w.count())
	assert.EqualValues(t, time3.Sub(time2), w.diff())
	assert.EqualValues(t, time3.Sub(time2), dt)
}

func TestFixedTimeWindow3(t *testing.T) {
	w := newFixedTimeWindow(3)
	assert.EqualValues(t, w.count(), 0)
	assert.EqualValues(t, w.diff(), zeroDuration)

	time1 := time.Unix(123456, 0)
	dt, full := w.add(time1)
	assert.False(t, full)
	assert.EqualValues(t, 1, w.count())
	assert.EqualValues(t, zeroDuration, w.diff())
	assert.EqualValues(t, zeroDuration, dt)

	time2 := time1.Add(time.Second)
	dt, full = w.add(time2)
	assert.False(t, full)
	assert.EqualValues(t, 2, w.count())
	assert.EqualValues(t, time2.Sub(time1), w.diff())
	assert.EqualValues(t, time2.Sub(time1), dt)

	time3 := time2.Add(2 * time.Second)
	dt, full = w.add(time3)
	assert.True(t, full)
	assert.EqualValues(t, 3, w.count())
	assert.EqualValues(t, time3.Sub(time1), w.diff())
	assert.EqualValues(t, time3.Sub(time1), dt)

	time4 := time3.Add(3 * time.Second)
	dt, full = w.add(time4)
	assert.True(t, full)
	assert.EqualValues(t, 3, w.count())
	assert.EqualValues(t, time4.Sub(time2), w.diff())
	assert.EqualValues(t, time4.Sub(time2), dt)
}

func TestFixedTimeWindowParallel(t *testing.T) {
	w := newFixedTimeWindow(10)

	wg := sync.WaitGroup{}
	wg.Add(2)
	go func() {
		for i := 0; i < 10000; i++ {
			w.add(time.Unix(int64(1234+i), 0))
		}
		wg.Done()
	}()

	go func() {
		for i := 0; i < 10000; i++ {
			w.add(time.Unix(int64(1234+i), 0))
		}
		wg.Done()
	}()

	wg.Wait()
	assert.EqualValues(t, 10, w.count())
}
