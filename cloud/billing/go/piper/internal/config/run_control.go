package config

import (
	"context"
	"os"
	"os/signal"
	"sync"
	"syscall"
)

type runControlContainer struct {
	runControlOnce sync.Once
	runCtx         context.Context
	runStop        context.CancelFunc
}

func (c *Container) GetRunContext() (context.Context, context.CancelFunc) {
	c.initRunControl()
	return c.runCtx, c.runStop
}

func (c *Container) initRunControl() {
	c.runControlOnce.Do(c.makeRunCtx)
}

func (c *Container) makeRunCtx() {
	runCtx, runStop := context.WithCancel(context.Background())

	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
	go func() {
		select {
		case <-sigChan:
			runStop()
		case <-runCtx.Done():
		}
	}()
	c.runCtx = runCtx
	c.runStop = runStop
}
