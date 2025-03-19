package consumer

import (
	"a.yandex-team.ru/cloud/netinfra/tflow/config"
	"a.yandex-team.ru/library/go/core/log"
	"bufio"
	"fmt"
	"os"
	"sync"
	"time"
)

type MessageType int

const (
	SFlowSample MessageType = iota
	SFlowEncap
	IpfixNatMessage
)

type MessageElement interface {
	fmt.Stringer
}

type Message struct {
	Type     MessageType
	Elements []MessageElement
}

type fileHandle struct {
	fd       *os.File
	bw       *bufio.Writer
	m        sync.Mutex
	fileName string
	logger   log.Logger
}

type Consumer struct {
	Queue  chan Message
	opts   *config.ConsumerCfg
	files  map[MessageType]*fileHandle
	logger log.Logger
	Reopen chan bool
}

func (fh *fileHandle) close() error {
	if fh.fd != nil {
		err := fh.bw.Flush()
		if err != nil {
			return err
		}
		err = fh.fd.Close()
		return err
	}
	return nil
}

func (fh *fileHandle) reopen() {
	fh.m.Lock()
	defer fh.m.Unlock()

	err := fh.close()
	if err != nil {
		fh.logger.Fatalf("failed to reopen %s: %v\n", fh.fileName, err)
	}
	fd, err := os.OpenFile(fh.fileName, os.O_CREATE|os.O_APPEND|os.O_WRONLY, 0644)
	if err != nil {
		panic(err)
	}
	fh.fd = fd
	fh.bw = bufio.NewWriter(fd)
}

func (fh *fileHandle) write(msg Message) (n int, err error) {
	fh.m.Lock()
	defer fh.m.Unlock()
	sum := 0
	for _, e := range msg.Elements {
		n, err := fh.bw.WriteString(e.String())
		sum += n
		if err != nil {
			return sum, err
		}
	}
	err = fh.bw.Flush()
	if err != nil {
		fh.logger.Errorf("failed to flush %s: %v\n", fh.fileName, err)
	}
	return sum, nil
}

// New create Consumer instance
func New(cfg *config.ConsumerCfg, logger log.Logger) *Consumer {
	retval := &Consumer{
		Queue:  make(chan Message, cfg.MsgQueue),
		opts:   cfg,
		files:  map[MessageType]*fileHandle{},
		logger: logger,
		Reopen: make(chan bool),
	}
	if cfg.SFlowLogFile != "" {
		retval.files[SFlowSample] = &fileHandle{fileName: cfg.SFlowLogFile, logger: logger}
	}
	if cfg.SFlowEncapLogFile != "" {
		retval.files[SFlowEncap] = &fileHandle{fileName: cfg.SFlowEncapLogFile, logger: logger}
	}
	if cfg.IpfixNATLogFile != "" {
		retval.files[IpfixNatMessage] = &fileHandle{fileName: cfg.IpfixNATLogFile, logger: logger}
	}
	return retval
}

func (c *Consumer) reopenLogs() {
	for _, v := range c.files {
		v.reopen()
	}
}

func (c *Consumer) closeLogs() {
	for _, v := range c.files {
		if err := v.close(); err != nil {
			c.logger.Errorf("consumer: closeLogs() : %v\n", err)
		}
	}
}

// closeLogs run Consumer
func (c *Consumer) Run() {
	c.reopenLogs()
	defer c.closeLogs()

	for {
		select {
		case <-c.Reopen:
			c.reopenLogs()
		case data, active := <-c.Queue:
			if !active {
				return
			}
			c.consume(data)
		}
	}
}

func (c *Consumer) consume(data Message) {
	fh, ok := c.files[data.Type]
	if !ok {
		c.logger.Warnf("consume(): unregistered message type %d\n", data.Type)
		return
	}
	if _, err := fh.write(data); err != nil {
		c.logger.Errorf("consume(): write %s: %s\n", fh.fileName, err.Error())
	}
}

func (c *Consumer) Shutdown() {
	for len(c.Queue) > 0 {
		time.Sleep(1 * time.Second)
	}
	c.logger.Infof("tskv queue size: %v\n", len(c.Queue))
}
