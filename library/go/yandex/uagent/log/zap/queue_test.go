package uazap

import (
	"runtime"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	uaclient "a.yandex-team.ru/library/go/yandex/uagent/log/zap/client"
)

func BenchmarkQueue(b *testing.B) {
	var q queue

	go func() {
		var buf []uaclient.Message

		for range time.Tick(10 * time.Millisecond) {
			buf = q.dequeueAll()
			queuePool.Put(buf)
		}
	}()

	var p uaclient.Message

	b.ReportAllocs()
	b.SetParallelism(runtime.NumCPU() - 1)
	b.RunParallel(func(pb *testing.PB) {
		for pb.Next() {
			q.enqueue(p)
		}
	})
}

func TestQueue(t *testing.T) {
	var b0, b1, b2, b3 uaclient.Message
	t0 := time.Now()
	b0 = uaclient.Message{Payload: []byte("b0"), Time: &t0, Meta: map[string]string{"hello": "world"}}
	t1 := time.Now()
	b1 = uaclient.Message{Payload: []byte("b1"), Time: &t1}
	t2 := time.Now()
	b2 = uaclient.Message{Payload: []byte("b2"), Time: &t2}
	t3 := time.Now()
	b3 = uaclient.Message{Payload: []byte("b3"), Time: &t3}

	var q queue
	q.enqueue(b0)
	q.enqueue(b1)
	q.enqueue(b2)

	require.Equal(t, []uaclient.Message{b0, b1, b2}, q.dequeueAll())

	q.enqueue(b0)
	q.enqueue(b1)
	q.enqueue(b2)
	q.enqueue(b3)

	require.Equal(t, []uaclient.Message{b0, b1, b2, b3}, q.dequeueAll())
}
