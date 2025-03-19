package collector

import (
	"net"
	"sync"
	"time"

	"a.yandex-team.ru/cloud/netinfra/tflow/config"
	"a.yandex-team.ru/cloud/netinfra/tflow/consumer"
	"a.yandex-team.ru/cloud/netinfra/tflow/ipfix"
	"a.yandex-team.ru/cloud/netinfra/tflow/sflow"
	"a.yandex-team.ru/cloud/netinfra/tflow/worker"
	"a.yandex-team.ru/library/go/core/log"
)

type Func func(j worker.Job, p chan *worker.Worker, logger log.Logger)

type Collector struct {
	consumer    *consumer.Consumer
	workerPool  chan *worker.Worker
	udpQueue    chan worker.UDPMsg
	udpSize     int
	con         *net.UDPConn
	logger      log.Logger
	stop        chan bool
	payloadPool *sync.Pool
	wFunc       worker.Func
	cFunc       Func
	dispatchWG  *sync.WaitGroup
	workersWG   *sync.WaitGroup
	udpWG       *sync.WaitGroup
}

func newCollectorCommon(cfg config.CollectorCfg, cons *consumer.Consumer, logger log.Logger) (*Collector, error) {
	addr, err := net.ResolveUDPAddr("udp", cfg.Listen)
	if err != nil {
		return nil, err
	}
	con, err := net.ListenUDP("udp", addr)
	if err != nil {
		return nil, err
	}

	return &Collector{
		consumer:   cons,
		workerPool: make(chan *worker.Worker, cfg.Workers),
		udpQueue:   make(chan worker.UDPMsg, cfg.UDPQueue),
		udpSize:    cfg.UDPSize,
		con:        con,
		logger:     logger,
		stop:       make(chan bool),
		payloadPool: &sync.Pool{
			New: func() interface{} {
				return make([]byte, cfg.UDPSize)
			},
		},
		dispatchWG: &sync.WaitGroup{},
		workersWG:  &sync.WaitGroup{},
		udpWG:      &sync.WaitGroup{},
	}, nil
}

func NewIpfix(cfg *config.IpfixCollectorCfg, cons *consumer.Consumer, logger log.Logger) (*Collector, error) {
	retval, err := newCollectorCommon(cfg.Cfg, cons, logger)
	if err != nil {
		return nil, err
	}
	retval.wFunc = ipfix.WorkerFunc
	retval.cFunc = ipfix.CollectorFunc
	return retval, nil
}

func NewSFlow(cfg *config.SFlowCollectorCfg, cons *consumer.Consumer, logger log.Logger) (*Collector, error) {
	retval, err := newCollectorCommon(cfg.Cfg, cons, logger)
	if err != nil {
		return nil, err
	}
	if cfg.Encap {
		retval.wFunc = sflow.EncapFunc
	} else {
		retval.wFunc = sflow.SampleFunc
	}
	retval.cFunc = sflow.CollectorFunc
	return retval, nil
}

// Run service
func (c *Collector) Run() {
	for i := 0; i < cap(c.workerPool); i++ {
		c.logger.Infof("Worker: %d\n", i)
		worker := worker.New(c.workerPool, c.consumer.Queue, c.wFunc, c.workersWG, c.logger)
		worker.Start()
	}

	c.dispatchWG.Add(1)
	go c.dispatch()

	c.udpWG.Add(1)
	go func() {
		defer c.udpWG.Done()
		for {
			select {
			case <-c.stop:
				return
			default:
				payload := c.payloadPool.Get().([]byte)
				length, addr, err := c.con.ReadFromUDP(payload)
				if err == nil {
					c.udpQueue <- worker.UDPMsg{Raddr: addr, Body: payload[:length]}
				} else {
					c.logger.Errorf("error reading from socket: %v\n", err)
				}
			}
		}
	}()
}

func (c *Collector) dispatch() {
	c.logger.Infof("Worker's queue dispatcher started.\n")
	defer c.dispatchWG.Done()
	for msg := range c.udpQueue {
		c.logger.Tracef("a dispatcher request received")
		c.cFunc(*worker.NewJob(msg, c.payloadPool, c.udpSize), c.workerPool, c.logger)
	}
}

// Shutdown service
func (c *Collector) Shutdown() {
	err := c.con.SetReadDeadline(time.Now())
	if err != nil {
		c.logger.Errorf("failed to SetReadDeadline: %v\n", err)
	}
	close(c.stop)
	c.udpWG.Wait()
	c.logger.Infof("collector graceful shutdown")
	// wait till udp queue is empty
	close(c.udpQueue)
	c.dispatchWG.Wait()
	c.logger.Infof("udp queue size: %v\n", len(c.udpQueue))

	go func() {
		for w := range c.workerPool {
			w.Stop()
		}
	}()
	c.workersWG.Wait()
	close(c.workerPool)
	c.logger.Infof("workers are stopped")
}
