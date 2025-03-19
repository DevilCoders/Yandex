package config

import (
	"context"
	"errors"
	"fmt"
	"sync"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/config/states"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logf"
	"a.yandex-team.ru/cloud/billing/go/pkg/errsentinel"
	"a.yandex-team.ru/library/go/core/log"
)

type Container struct {
	mainCtx   context.Context
	initError error
	failOnce  sync.Once

	logger log.Structured

	statesMgr states.Manager

	authContainer
	chContainer
	cloudContainer
	configContainer
	configOverrideContainer
	interconnectContainer
	runControlContainer
	statusContainer
	tlsContainer
	toolingContainer
	ydbContainer

	dumperContainer
	resharderContainer
}

type OverrideFlag bool

const (
	WithOverride OverrideFlag = true
	NoOverride   OverrideFlag = false
)

func NewContainer(ctx context.Context, allowOverride OverrideFlag, configPath ...string) *Container {
	mainCtx, _ := tooling.InitContext(ctx, "init")
	c := Container{mainCtx: mainCtx}
	c.files = configPath
	c.overrideConfig = bool(allowOverride)
	return &c
}

func (c *Container) Shutdown() error {
	_, stop := c.GetRunContext()
	defer stop()
	return c.statesMgr.Shutdown()
}

var ErrInit = errsentinel.New("configuration initialization")

func (c *Container) initFailed() bool {
	return c.initError != nil
}

func (c *Container) failInit(err error) error {
	if err == nil {
		return err
	}
	if !errors.Is(err, ErrInit) {
		err = ErrInit.Wrap(err)
	}
	c.setInitError(err)
	return err
}

func (c *Container) failInitF(msg string, args ...interface{}) error {
	return c.failInit(fmt.Errorf(msg, args...))
}

func (c *Container) setInitError(err error) {
	c.failOnce.Do(func() { c.initError = err })
}

func (c *Container) dummy() dummyImpl {
	return dummyImpl{ctx: c.mainCtx}
}

var initLogService = logf.Service("init")

func (c *Container) debug(msg string, fld ...log.Field) {
	if c.logger == nil {
		return
	}
	fields := append([]log.Field{initLogService}, fld...)
	c.logger.Debug(msg, fields...)
}
