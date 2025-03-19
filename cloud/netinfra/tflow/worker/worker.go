package worker

import (
	"a.yandex-team.ru/cloud/netinfra/tflow/consumer"
	"a.yandex-team.ru/cloud/netinfra/tflow/metrics"
	"a.yandex-team.ru/library/go/core/log"
	"github.com/prometheus/client_golang/prometheus"
	"net"
	"sync"
	"sync/atomic"
)

type UDPMsg struct {
	Raddr *net.UDPAddr
	Body  []byte
}

type Job struct {
	Payload  UDPMsg
	Extra    interface{}
	storage  []byte
	refcount *int32
	pool     *sync.Pool
	size     int
}

func (j *Job) Teardown() {
	atomic.AddInt32(j.refcount, -1)
	if *j.refcount == 0 {
		j.storage = j.storage[:j.size]
		j.pool.Put(j.storage)
	}
}

func NewJob(msg UDPMsg, p *sync.Pool, s int) *Job {
	rcount := new(int32)
	*rcount = 1
	return &Job{
		Payload:  msg,
		Extra:    nil,
		storage:  msg.Body,
		refcount: rcount,
		pool:     p,
		size:     s,
	}
}

func NewChildJob(j Job, start uint16, stop uint16) *Job {
	atomic.AddInt32(j.refcount, 1)
	return &Job{
		Payload:  UDPMsg{Raddr: j.Payload.Raddr, Body: j.Payload.Body[start:stop]},
		Extra:    j.Extra,
		storage:  j.storage,
		refcount: j.refcount,
		pool:     j.pool,
		size:     j.size,
	}
}

type Func func(j Job, out chan consumer.Message, log log.Logger)

type Worker struct {
	WorkerPool chan *Worker
	JobChannel chan Job
	OutputQ    chan consumer.Message
	execute    Func
	quit       chan bool
	wg         *sync.WaitGroup
	logger     log.Logger
}

func New(
	workerPool chan *Worker,
	outputQ chan consumer.Message,
	wfunc Func,
	wg *sync.WaitGroup,
	logger log.Logger) Worker {

	return Worker{
		WorkerPool: workerPool,
		JobChannel: make(chan Job),
		OutputQ:    outputQ,
		execute:    wfunc,
		quit:       make(chan bool),
		wg:         wg,
		logger:     logger,
	}
}

func (w Worker) Start() {
	w.wg.Add(1)
	go func() {
		defer w.wg.Done()
		w.WorkerPool <- &w
		for job := range w.JobChannel {
			msg := job.Payload
			w.execute(job, w.OutputQ, w.logger)
			metrics.MetricTrafficBytes.With(
				prometheus.Labels{
					"remote_ip": msg.Raddr.IP.String(),
				},
			).Add(float64(len(msg.Body)))
			metrics.MetricTrafficPackets.With(
				prometheus.Labels{
					"remote_ip": msg.Raddr.IP.String(),
				},
			).Inc()
			metrics.MetricPacketSizeSum.With(
				prometheus.Labels{
					"remote_ip": msg.Raddr.IP.String(),
				},
			).Observe(float64(len(msg.Body)))
			job.Teardown()
			w.WorkerPool <- &w
		}
	}()
}

func (w Worker) Stop() {
	close(w.JobChannel)
}
