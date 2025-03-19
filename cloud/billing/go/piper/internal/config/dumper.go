package config

import (
	"fmt"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/adapters/logbroker"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/servers/logbroker/handlers"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logf"
	"a.yandex-team.ru/cloud/billing/go/pkg/logbroker/lbtypes"
)

type dumperContainer struct {
	dumperOnce initSync

	dumperServices map[string]Runner

	dumperSinkLBOnce initSync
	dumperSinkLB     *logbroker.Adapter
}

func (c *Container) initDumper() error {
	return c.dumperOnce.Do(c.makeDumper)
}

func (c *Container) makeDumper() error {
	if c.initFailed() {
		return c.initError
	}

	c.debug("constructing dumper")
	cfg, err := c.GetDumperConfig()
	if err != nil {
		return err
	}
	if cfg.Disabled.Bool() {
		c.debug("dumper disabled")
		return nil
	}

	c.dumperServices = make(map[string]Runner)
	for srcName, src := range cfg.Source {
		if src == nil || src.Disabled.Bool() {
			continue
		}
		nameField := logf.SourceName(srcName)
		c.debug("found active dumper source", nameField, logf.Handler(src.Handler))
		handler, err := c.makeDumperHandler(srcName, src.Handler)
		if err != nil {
			return c.failInitF("build dumper source %s: %w", srcName, err)
		}
		c.debug("constructing dumper logbroker service", nameField)
		srv, err := c.makeLogbrokerService(
			srcName, 0, src.Logbroker, constantConsumer(src.Logbroker.Consumer),
			handler, lbtypes.ReadCommittedOffsets,
		)
		if err != nil {
			return err
		}
		c.debug("dumper logbroker service constructed", nameField)
		instName := fmt.Sprintf("%s:%d", srcName, 0)
		c.statesMgr.AddCritical(instName, srv)
		c.debug("dumper logbroker service state registered", nameField, logf.Value(instName))
		c.dumperServices[instName] = srv
	}

	return nil
}

func (c *Container) makeDumperHandler(srcName string, name string) (lbtypes.Handler, error) {
	dCfg, _ := c.GetDumperConfig()

	switch name {
	case "dumpErrors:ydb":
		if !dCfg.Sink.YDBErrors.Enabled.Bool() {
			return nil, fmt.Errorf("ydb sink should be enabled for handler %s", name)
		}
		return c.makeYDBDumpErrorsHandler(srcName, name)
	case "dumpErrors:clickhouse":
		if !dCfg.Sink.ClickhouseErrors.Enabled.Bool() {
			return nil, fmt.Errorf("clickhouse sink should be enabled for handler %s", name)
		}
		return c.makeChDumpErrorsHandler(srcName, name)
	case "presenter:ydb":
		return nil, fmt.Errorf("presenter not ready yet, follow to CLOUD-86683")
		//if !dCfg.Sink.YDBPresenter.Enabled.Bool() {
		//	return nil, fmt.Errorf("presenter sink should be enabled for handler %s", name)
		//}
		//return c.makeYDBDumpPresenterHandler(srcName, name)
	}
	return nil, fmt.Errorf("unknown dumper handler %s", name)
}

func (c *Container) makeYDBDumpErrorsHandler(srcName string, name string) (lbtypes.Handler, error) {
	ydba, err := c.GetYDBAdapter()
	if err != nil {
		return nil, err
	}
	targetFabric := func() handlers.DumpErrorsTarget {
		return ydba.Session()
	}
	return handlers.NewDumpErrorsHandler(srcName, name, "dump", targetFabric), nil
}

func (c *Container) makeChDumpErrorsHandler(srcName string, name string) (lbtypes.Handler, error) {
	cha, err := c.GetClickhouseAdapter()
	if err != nil {
		return nil, err
	}
	targetFabric := func() handlers.DumpErrorsTarget {
		return cha.Session()
	}
	return handlers.NewDumpErrorsHandler(srcName, name, "dump", targetFabric), nil
}

func (c *Container) makeYDBDumpPresenterHandler(srcName string, name string) (lbtypes.Handler, error) {
	ydba, err := c.GetYDBPresenterAdapter()
	if err != nil {
		return nil, err
	}
	targetFabric := func() handlers.YDBPresenterTarget {
		return ydba.PresenterSession()
	}
	return handlers.NewYDBPresenterHandler(srcName, name, "presenter", targetFabric), nil
}
