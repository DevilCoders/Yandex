package worker

import (
	"a.yandex-team.ru/cloud/netinfra/tflow/consumer"
	"log"
	"net"
	"sync"
	"testing"
	"time"
)

type testVal struct {
	str string
}

func (t testVal) String() string {
	return t.str
}

func workerEq(left *Worker, right *Worker) bool {
	return (left.WorkerPool == right.WorkerPool &&
		left.JobChannel == right.JobChannel &&
		left.OutputQ == right.OutputQ)
}

func TestWorker(t *testing.T) {
	wpool := make(chan *Worker, 2)
	mqueue := make(chan consumer.Message)
	reply := "done"
	wg := &sync.WaitGroup{}
	wg.Add(1)
	w := New(
		wpool,
		mqueue,
		func(j Job, out chan consumer.Message, logger *log.Logger) {
			var retval = make([]consumer.MessageElement, 1)
			retval[0] = testVal{reply}
			out <- consumer.Message{consumer.IpfixNatMessage, retval}
		},
		wg,
		nil,
	)
	w.Start()
	j := NewJob(
		UDPMsg{
			Raddr: &net.UDPAddr{net.IPv4(10, 0, 0, 1), 4077, ""},
			Body:  []byte{},
		},
		&sync.Pool{},
		0,
	)

	var timeout time.Duration = 1 * time.Second
	tchan := time.After(timeout)
	select {
	case <-tchan:
		t.Errorf("no worker registered in worker pool")
		return
	case worker := <-wpool:
		if !workerEq(worker, &w) {
			t.Errorf("got unknown worker %v while waiting for %v", *worker, w)
			return
		}
		worker.JobChannel <- *j
	}
	tchan = time.After(timeout)
	select {
	case <-tchan:
		t.Errorf("no consumer message received. Worker hung?")
		return
	case m := <-mqueue:
		if m.Type != consumer.IpfixNatMessage {
			t.Errorf("got %v message type while expected %v", m.Type, consumer.IpfixNatMessage)
		}
		for i, e := range m.Elements {
			if e.String() != reply {
				t.Errorf("element #%d got '%s' expected '%s'", i, e.String(), reply)
			}
		}
	}
}
