package internal

import "time"

type Clock interface {
	Now() int64
	TimeoutPassed(timestamp int64, timeout time.Duration) bool
}

const TimestampEmpty int64 = -1

type RealClock struct{}

func (*RealClock) Now() int64 { return time.Now().UTC().Unix() }

func (c *RealClock) TimeoutPassed(timestamp int64, timeout time.Duration) bool {
	timeoutSeconds := int64(timeout / time.Second)

	return timestamp+timeoutSeconds >= c.Now()
}
