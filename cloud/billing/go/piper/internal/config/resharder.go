package config

import (
	"fmt"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/adapters/logbroker"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/config/cfgtypes"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/servers/logbroker/handlers"
	"a.yandex-team.ru/cloud/billing/go/pkg/logbroker/lbtypes"
)

type resharderContainer struct {
	resharderOnce initSync

	resharderServices map[string]Runner

	resharderSinkLBOnce initSync
	resharderSinkLB     *logbroker.Adapter
	resharderLBWriters  logbroker.Writers
}

func (c *Container) initResharder() error {
	return c.resharderOnce.Do(c.makeResharder)
}

func (c *Container) makeResharder() error {
	if c.initFailed() {
		return c.initError
	}

	cfg, err := c.GetResharderConfig()
	if err != nil {
		return err
	}
	if cfg.Disabled.Bool() {
		return nil
	}

	c.resharderServices = make(map[string]Runner)
	for srcName, src := range cfg.Source {
		if src == nil || src.Disabled.Bool() || src.Handler == "" { // NOTE: check handler for cases load from lockbox without actual config in files
			continue
		}
		parallel := 1
		if src.Parallel > 0 {
			parallel = src.Parallel
		}
		for i := 0; i < parallel; i++ {
			handler, offsets, err := c.makeResharderHandler(srcName, src.Handler, src.Params)
			if err != nil {
				return c.failInitF("build resharder source %s: %w", srcName, err)
			}
			srv, err := c.makeLogbrokerService(
				srcName, i, src.Logbroker, resharderConsumer(src.Logbroker.Consumer),
				handler, offsets,
			)
			if err != nil {
				return err
			}
			instName := fmt.Sprintf("resharder-lbreader:%s:%d", srcName, i)
			c.statesMgr.Add(instName, srv)
			c.resharderServices[instName] = srv
		}
	}

	return nil
}

func (c *Container) makeResharderHandler(
	srcName string, name string, cfg cfgtypes.ResharderHandlerConfig,
) (lbtypes.Handler, lbtypes.OffsetReporter, error) {
	rCfg, _ := c.GetResharderConfig()
	hCfg := handlers.ResharderHandlerConfig{
		ChunkSize:      cfg.ChunkSize.Int(),
		MetricLifetime: cfg.MetricLifetime.Duration(),
		MetricGrace:    cfg.MetricGrace.Duration(),
	}

	fabrics := handlers.ResharderFabrics{
		OversizedMessagePusherFabric: c.oversizedMessagePusherFabric(),
		MetricsResolverFabric:        c.metricsResolverFabric(),
		BillingAccountsGetterFabric:  c.billingAccountsGetterFabric(),
		IdentityResolverFabric:       c.identityResolverFabric(),
		AbcResolverFabric:            c.abcResolverFabric(rCfg.EnableTeamIntegration.Bool()),
		MetricsPusherFabric:          c.metricsPusherFabric(),
		CumulativeCalculatorFabric:   c.cumulativeCalculatorFabric(),
		DuplicatesSeekerFabric:       c.duplicatesSeekerFabric(rCfg.EnableDeduplication.Bool()),
		E2EPusherFabric:              c.e2ePusherFabric(rCfg.EnableE2EReporting.Bool()),
	}
	if c.initFailed() {
		return nil, nil, c.initError
	}

	switch name {
	case "general":
		handler := handlers.NewGeneralResharderHandler(
			name, "resharder", srcName, hCfg, fabrics,
		)
		return handler, c.resharderLBWriters.ReshardedMetrics, nil
	case "aggregating":
		handler := handlers.NewAggregatingResharderHandler(
			name, "resharder", srcName, hCfg, fabrics,
		)
		return handler, c.resharderLBWriters.ReshardedMetrics, nil
	case "cumulative":
		handler := handlers.NewCumulativeResharderHandler(
			name, "resharder", srcName, hCfg, fabrics,
		)
		return handler, c.resharderLBWriters.ReshardedMetrics, nil
	case "cumulative-prorated":
		handler := handlers.NewCumulativeProratedResharderHandler(
			name, "resharder", srcName, hCfg, fabrics,
		)
		return handler, c.resharderLBWriters.ReshardedMetrics, nil
	}
	return nil, nil, fmt.Errorf("unknown resharder handler %s", name)
}

func (c *Container) initResharderSinkLB() error {
	return c.resharderSinkLBOnce.Do(c.makeResharderSinkLB)
}

func (c *Container) makeResharderSinkLB() error {
	cfg, err := c.GetResharderConfig()
	if err != nil {
		return err
	}

	if c.resharderLBWriters.ReshardedMetrics, err = c.makeLBWriter(cfg.Sink.Logbroker); err != nil {
		return c.failInitF("resharder logbroker writer: %w", err)
	}
	if c.resharderLBWriters.IncorrectMetrics, err = c.makeLBWriter(cfg.Sink.LogbrokerErrors); err != nil {
		return c.failInitF("resharder logbroker errors writer: %w", err)
	}
	if c.resharderSinkLB, err = logbroker.New(c.mainCtx, c.resharderLBWriters); err != nil {
		return c.failInitF("resharder logbroker adapter: %w", err)
	}
	c.resharderSinkLB.SetInflyLimit(cfg.WriteLimit)
	c.statesMgr.Add("resharder-sink-lb", c.resharderSinkLB)
	return nil
}

// fabrics
func (c *Container) oversizedMessagePusherFabric() func() handlers.OversizedMessagePusher {
	ydba, _ := c.GetYDBAdapter()
	return func() handlers.OversizedMessagePusher {
		return ydba.Session()
	}
}

func (c *Container) metricsResolverFabric() func() handlers.MetricsResolver {
	ydba, _ := c.GetYDBAdapter()
	return func() handlers.MetricsResolver {
		return ydba.Session()
	}
}

func (c *Container) billingAccountsGetterFabric() func() handlers.BillingAccountsGetter {
	ydba, _ := c.GetYDBAdapter()
	return func() handlers.BillingAccountsGetter {
		return ydba.Session()
	}
}

func (c *Container) identityResolverFabric() func() handlers.IdentityResolver {
	iama, _ := c.GetIAMAdapter()
	return func() handlers.IdentityResolver {
		return iama.Session()
	}
}

func (c *Container) abcResolverFabric(enabled bool) func() handlers.AbcResolver {
	if !enabled {
		dummy := c.dummy()
		return func() handlers.AbcResolver { return dummy }
	}
	tia, _ := c.GetTIAdapter()
	return func() handlers.AbcResolver {
		return tia.Session()
	}
}

func (c *Container) metricsPusherFabric() func() handlers.MetricsPusher {
	if err := c.initResharderSinkLB(); err != nil {
		return nil
	}
	return func() handlers.MetricsPusher {
		return c.resharderSinkLB.Session()
	}
}

func (c *Container) cumulativeCalculatorFabric() func() handlers.CumulativeCalculator {
	ydba, _ := c.GetYDBCumulativeAdapter()
	return func() handlers.CumulativeCalculator {
		return ydba.Session()
	}
}

func (c *Container) duplicatesSeekerFabric(enabled bool) func() handlers.DuplicatesSeeker {
	if !enabled {
		dummy := c.dummy()
		return func() handlers.DuplicatesSeeker { return dummy }
	}

	ydbUniq, _ := c.GetYDBUniqAdapter()
	return func() handlers.DuplicatesSeeker {
		return ydbUniq.UniqSession()
	}
}

func (c *Container) e2ePusherFabric(enabled bool) func() handlers.E2EPusher {
	if !enabled {
		dummy := c.dummy()
		return func() handlers.E2EPusher { return dummy }
	}

	uaa, _ := c.GetUAAdapter()
	return func() handlers.E2EPusher {
		return uaa.Session()
	}
}
