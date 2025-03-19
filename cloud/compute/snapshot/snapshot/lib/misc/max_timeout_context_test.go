package misc

import (
	"context"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"go.uber.org/atomic"
)

func TestMaxTimeoutContext(t *testing.T) {
	at := assert.New(t)

	base, cancelBase := context.WithCancel(context.Background())
	start := time.Now()
	ctx := WithNoCancelIn(base, time.Second/10)
	cancelBase()
	at.NoError(ctx.Err())
	deadline, ok := ctx.Deadline()
	at.Equal(time.Time{}, deadline)
	at.False(ok)
	<-ctx.Done()
	at.True(time.Since(start) >= time.Second/10)
	at.Error(ctx.Err())
}

func TestCallAfterBothClosed(t *testing.T) {
	at := assert.New(t)
	pause := func() { time.Sleep(time.Millisecond * 50) }

	// close c1 then c2
	flag := atomic.NewBool(false)
	flagFunc := func() { flag.Store(true) }
	c1 := make(chan time.Time)
	c2 := make(chan struct{})
	go callAfterBothClosed(flagFunc, c1, c2)
	at.False(flag.Load())

	close(c1)
	pause()
	at.False(flag.Load())

	close(c2)
	pause()
	at.True(flag.Load())

	// close c2 then c1
	flag = atomic.NewBool(false)
	c1 = make(chan time.Time)
	c2 = make(chan struct{})
	go callAfterBothClosed(flagFunc, c1, c2)
	at.False(flag.Load())

	close(c2)
	pause()
	at.False(flag.Load())

	close(c1)
	pause()
	at.True(flag.Load())
}

func TestWithNoCancel(t *testing.T) {
	type testType struct{}
	ctx, cancel := context.WithCancel(context.WithValue(context.Background(), testType{}, 2))
	cancel()

	ctx = WithNoCancel(ctx)
	at := assert.New(t)
	at.Nil(ctx.Done())
	at.NoError(ctx.Err())
	at.Equal(2, ctx.Value(testType{}))
}
