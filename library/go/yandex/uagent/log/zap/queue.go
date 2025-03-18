package uazap

import (
	"sync"

	uaclient "a.yandex-team.ru/library/go/yandex/uagent/log/zap/client"
)

var queuePool = sync.Pool{
	New: func() interface{} {
		return make([]uaclient.Message, 10)
	},
}

type queue struct {
	buf  []uaclient.Message
	lock sync.Mutex
}

func (q *queue) enqueue(message uaclient.Message) {
	q.lock.Lock()
	defer q.lock.Unlock()

	q.buf = append(q.buf, message)
}

func (q *queue) dequeueAll() []uaclient.Message {
	q.lock.Lock()
	defer q.lock.Unlock()

	buf := q.buf
	q.buf = queuePool.Get().([]uaclient.Message)
	q.buf = q.buf[:0]

	return buf
}
