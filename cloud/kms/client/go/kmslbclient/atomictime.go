package kmslbclient

import (
	"sync/atomic"
	"time"
)

type atomicTime struct {
	v int64
}

func (d *atomicTime) Store(value time.Time) {
	atomic.StoreInt64(&d.v, value.Unix())
}

func (d *atomicTime) Load() time.Time {
	v := atomic.LoadInt64(&d.v)
	return time.Unix(v, 0)
}

func (d *atomicTime) String() string {
	return d.Load().String()
}
