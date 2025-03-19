package config

import (
	"context"
	"errors"

	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/adapters/iam"
	teamintegration "a.yandex-team.ru/cloud/billing/go/piper/internal/adapters/team_integration"
	unifiedagent "a.yandex-team.ru/cloud/billing/go/piper/internal/adapters/unified_agent"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/config/cfgtypes"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/grpccon"
	iam_ic "a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/iam"
	teamintegration_ic "a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/team_integration"
	unifiedagent_ic "a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/unified_agent"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logf"
)

type GRPCProvider func(string, cfgtypes.AuthConfig, cfgtypes.GRPCConfig) (grpc.ClientConnInterface, error)

type interconnectContainer struct {
	rmOnce         initSync
	rmConnProvider GRPCProvider
	rmClient       iam_ic.RMClient

	tiOnce         initSync
	tiConnProvider GRPCProvider
	tiClient       teamintegration_ic.TIClient

	iamAdapterOnce initSync
	iamAdapter     *iam.Adapter

	tiAdapterOnce initSync
	tiAdapter     *teamintegration.Adapter

	uaAdapterOnce initSync
	uaOnce        initSync
	uaAdapter     *unifiedagent.Adapter
	uaClient      unifiedagent_ic.UAClient
}

func (c *Container) GetIAMAdapter() (*iam.Adapter, error) {
	err := c.initIAMAdapter()
	return c.iamAdapter, err
}

func (c *Container) GetTIAdapter() (*teamintegration.Adapter, error) {
	err := c.initTIAdapter()
	return c.tiAdapter, err
}

func (c *Container) GetUAAdapter() (*unifiedagent.Adapter, error) {
	err := c.initUAAdapter()
	return c.uaAdapter, err
}

func (c *Container) initIAMAdapter() error {
	return c.iamAdapterOnce.Do(c.makeIAMAdapter)
}

func (c *Container) makeIAMAdapter() error {
	c.initializedTooling()
	c.connectedRM()
	if c.initFailed() {
		return c.initError
	}

	c.debug("constructing iam adapter")
	iamAdapter, err := iam.New(c.mainCtx, c.rmClient)
	if err != nil {
		return c.failInitF("IAM adapter init: %w", err)
	}
	c.statesMgr.Add("iam-adapter", iamAdapter)
	c.debug("iam-adapter state registered")

	c.iamAdapter = iamAdapter
	return nil
}

func (c *Container) connectedRM() {
	_ = c.rmOnce.Do(c.connectRM)
}

func (c *Container) connectRM() error {
	c.initializedTooling()
	if c.initFailed() {
		return c.initError
	}

	cfg, err := c.GetRMConfig()
	if err != nil {
		return err
	}
	if cfg.Endpoint == "" {
		return c.failInit(errors.New("rm is not configured"))
	}

	prov := c.rmConnProvider
	if prov == nil {
		prov = c.grpcConn
	}
	c.debug("connecting to RM")
	conn, err := prov(cfg.Endpoint, cfg.Auth, cfg.GRPC)
	if err != nil {
		return c.failInitF("rm connect: %w", err)
	}
	c.debug("connected to RM")

	rm := iam_ic.NewRMClient(c.mainCtx, conn)
	c.statesMgr.AddCritical("iam", rm)
	c.debug("iam state registered")

	if err := rm.HealthCheck(context.Background()); err != nil {
		return c.failInitF("rm access: %w", err)
	}
	c.rmClient = rm
	return nil
}

func (c *Container) initTIAdapter() error {
	return c.iamAdapterOnce.Do(c.makeTIAdapter)
}

func (c *Container) makeTIAdapter() error {
	c.initializedTooling()
	c.connectedTI()
	if c.initFailed() {
		return c.initError
	}

	c.debug("constructing team-integration adapter")
	tiAdapter, err := teamintegration.New(c.mainCtx, c.tiClient)
	if err != nil {
		return c.failInitF("Team integration adapter init: %w", err)
	}
	c.statesMgr.Add("team-integration-adapter", tiAdapter)
	c.debug("team-integration-adapter state registered")
	c.tiAdapter = tiAdapter
	return nil
}

func (c *Container) connectedTI() {
	_ = c.tiOnce.Do(c.connectTI)
}

func (c *Container) connectTI() error {
	c.initializedTooling()
	if c.initFailed() {
		return c.initError
	}

	cfg, err := c.GetTIConfig()
	if err != nil {
		return err
	}
	if cfg.Endpoint == "" {
		return c.failInit(errors.New("team integration is not configured"))
	}

	prov := c.tiConnProvider
	if prov == nil {
		prov = c.grpcConn
	}
	c.debug("connecting to TI")
	conn, err := prov(cfg.Endpoint, cfg.Auth, cfg.GRPC)
	if err != nil {
		return c.failInitF("team integration connect: %w", err)
	}
	c.debug("connected to TI")

	ti := teamintegration_ic.NewTIClient(c.mainCtx, conn)
	c.statesMgr.Add("team-integration", ti)
	c.debug("team-integration state registered")
	if err := ti.HealthCheck(context.Background()); err != nil {
		return c.failInitF("team integration access: %w", err)
	}
	c.tiClient = ti
	return nil
}

func (c *Container) initUAAdapter() error {
	return c.uaAdapterOnce.Do(c.makeUAAdapter)
}

func (c *Container) makeUAAdapter() error {
	c.initializedTooling()
	c.connectedUA()
	if c.initFailed() {
		return c.initError
	}

	c.debug("constructing unified-agent adapter")
	uaAdapter, err := unifiedagent.New(c.mainCtx, c.uaClient)
	if err != nil {
		return c.failInitF("Unified agent adapter init: %w", err)
	}
	c.statesMgr.Add("unified-agent-adapter", uaAdapter)
	c.debug("unified-agent-adapter state registered")
	c.uaAdapter = uaAdapter
	return nil
}

func (c *Container) connectedUA() {
	_ = c.uaOnce.Do(c.connectUA)
}

func (c *Container) connectUA() error {
	c.initializedTooling()
	if c.initFailed() {
		return c.initError
	}

	cfg, err := c.GetUAConfig()
	if err != nil {
		return err
	}
	if cfg.SolomonMetricsPort == 0 || cfg.HealthCheckPort == 0 {
		return c.failInit(errors.New("unified agent is not configured"))
	}

	ua := unifiedagent_ic.NewUAClient(cfg.SolomonMetricsPort, cfg.HealthCheckPort)
	c.statesMgr.AddCritical("ua", ua)
	c.debug("ua state registered")

	if err := ua.HealthCheck(context.Background()); err != nil {
		return c.failInitF("ua access: %w", err)
	}
	c.uaClient = ua
	return nil
}

func (c *Container) grpcConn(
	endpoint string, authCfg cfgtypes.AuthConfig, grpcCfg cfgtypes.GRPCConfig,
) (grpc.ClientConnInterface, error) {
	c.debug("grpc endpoint", logf.Value(endpoint))
	c.debug("grpc auth type", logf.Value(authCfg.Type.String()))
	auth, err := c.GetGRPCAuth(authCfg)
	if err != nil {
		return nil, err
	}

	config := grpccon.Config{}
	if !grpcCfg.DisableTLS.Bool() {
		c.debug("grpc tls enabled")
		tls, err := c.GetTLS()
		if err != nil {
			return nil, err
		}
		config.TLS = tls
	}
	config.KeepAliveConfig.Time = grpcCfg.KeepAliveTime.Duration()
	config.KeepAliveConfig.Timeout = grpcCfg.KeepAliveTimeout.Duration()
	config.RetryConfig.MaxRetries = grpcCfg.RequestRetries
	config.RetryConfig.PerRetryTimeout = grpcCfg.RequestRetryTimeout.Duration()

	return grpccon.Connect(c.mainCtx, endpoint, config, auth)
}
