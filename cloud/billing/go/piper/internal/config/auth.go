package config

import (
	"context"
	"fmt"
	"net/http"
	"sync"
	"time"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/config/cfgtypes"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/auth"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/auth/cloudjwt"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/auth/cloudmeta"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/auth/tvmticket"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/grpccon"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logf"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logging"
	"a.yandex-team.ru/library/go/yandex/tvm"
	"a.yandex-team.ru/library/go/yandex/tvm/tvmtool"
)

type authContainer struct {
	authMetaOnce initSync
	authJWTOnce  initSync
	authTVMLock  sync.Mutex

	metaAuth *cloudmeta.Authenticator
	jwtAuth  *cloudjwt.Authenticator
	tvmAuths map[uint]*tvmticket.Authenticator

	tvmClientOnce initSync
	tvmClient     tvm.Client
}

func (c *Container) GetYDBAuth(t cfgtypes.AuthConfig) (auth.YDBAuthenticator, error) {
	auth, err := c.getAuth(t)
	if err != nil {
		return nil, err
	}
	return auth.YDBAuth(), nil
}

func (c *Container) GetGRPCAuth(t cfgtypes.AuthConfig) (auth.GRPCAuthenticator, error) {
	auth, err := c.getAuth(t)
	if err != nil {
		return nil, err
	}
	return auth.GRPCAuth(), nil
}

type authenticator interface {
	GRPCAuth() auth.GRPCAuthenticator
	YDBAuth() auth.YDBAuthenticator
}

func (c *Container) getAuth(t cfgtypes.AuthConfig) (authenticator, error) {
	switch t.Type {
	case cfgtypes.NoAuthType:
		return c.dummy(), nil
	case cfgtypes.DefaultAuthType, cfgtypes.IAMMetaAuthType:
		if err := c.initMetaAuth(); err != nil {
			return nil, err
		}
		return c.metaAuth, nil
	case cfgtypes.JWTAuthType:
		if err := c.initJWTAuth(); err != nil {
			return nil, err
		}
		return c.jwtAuth, nil
	case cfgtypes.TVMAuthType:
		if err := c.initTVMAuth(t.TVMDestination); err != nil {
			return nil, err
		}
		return c.tvmAuths[t.TVMDestination], nil
	}
	return nil, c.failInitF("unknown auth type: %s", t.Type.String())
}

func (c *Container) initMetaAuth() error {
	return c.authMetaOnce.Do(c.makeMetaAuth)
}

func (c *Container) makeMetaAuth() error {
	c.initializedTooling()
	if c.initFailed() {
		return c.initError
	}

	c.debug("meta auth required")
	cfg, err := c.GetIAMMetaConfig()
	if err != nil {
		return err
	}

	if cfg.UseLocalhost.Bool() {
		c.debug("using localhost metadata for authentication")
		c.metaAuth = cloudmeta.NewLocal(c.mainCtx)
	} else {
		c.debug("using remote metadata for authentication")
		c.metaAuth = cloudmeta.New(c.mainCtx)
	}
	c.statesMgr.Add("iam-meta", c.metaAuth)
	c.debug("iam-meta state registered")

	if err := c.metaAuth.HealthCheck(context.Background()); err != nil {
		return c.failInitF("metadata auth: %w", err)
	}

	return nil
}

func (c *Container) initJWTAuth() error {
	return c.authJWTOnce.Do(c.makeJWTAuth)
}

func (c *Container) makeJWTAuth() error {
	c.initializedTooling()
	if c.initFailed() {
		return c.initError
	}

	c.debug("jwt auth required")
	cfg, err := c.GetJWTConfig()
	if err != nil {
		return err
	}

	tls, err := c.GetTLS()
	if err != nil {
		return err
	}

	grpcCfg := grpccon.Config{TLS: tls}

	c.debug("jwt auth grpc endpoint", logf.Value(cfg.Endpoint))

	conn, err := grpccon.Connect(c.mainCtx, cfg.Endpoint, grpcCfg, nil)
	if err != nil {
		return c.failInitF("iam jwt connection: %w", err)
	}

	// Load secrets from Key File (key.json)
	key := cfg.Key
	if cfg.KeyFile != "" {
		key, err = c.GetJWTKeyConfig(cfg.KeyFile)
		if err != nil {
			return err
		}
	}

	c.debug("jwt auth grpc connected")
	c.debug("jwt auth audience used", logf.Value(cfg.Audience))
	c.jwtAuth, err = cloudjwt.New(c.mainCtx, conn, cloudjwt.Config{
		Audience:   cloudjwt.Audience(cfg.Audience),
		AccountID:  key.ServiceAccountID,
		KeyID:      key.ID,
		PrivateKey: []byte(key.PrivateKey),
	})
	if err != nil {
		return c.failInitF("jwt auth create: %w", err)
	}
	c.statesMgr.Add("jwt", c.jwtAuth)
	c.debug("jwt auth state registered")

	if err := c.jwtAuth.HealthCheck(context.Background()); err != nil {
		return c.failInitF("jwt auth: %w", err)
	}

	return nil
}

func (c *Container) initTVMAuth(dst uint) error {
	c.authTVMLock.Lock()
	defer c.authTVMLock.Unlock()
	if _, ok := c.tvmAuths[dst]; ok {
		return nil
	}
	return c.makeTVMAuth(dst)
}

func (c *Container) makeTVMAuth(dst uint) error {
	c.initializedTooling()
	c.initializedTVMClient()
	if c.initFailed() {
		return c.initError
	}

	c.debug("tvm auth required")
	c.debug("tvm destination id", logf.Value(fmt.Sprintf("%d", dst)))
	auth := tvmticket.New(c.mainCtx, c.tvmClient, tvm.ClientID(dst))
	c.statesMgr.Add(fmt.Sprintf("tvm-%d", dst), auth)
	c.debug("tvm auth state registered")

	if err := auth.HealthCheck(context.Background()); err != nil {
		return c.failInitF("tvm auth for dst %d: %w", dst, err)
	}

	if c.tvmAuths == nil {
		c.tvmAuths = make(map[uint]*tvmticket.Authenticator)
	}

	c.tvmAuths[dst] = auth
	return nil
}

func (c *Container) initializedTVMClient() {
	_ = c.authJWTOnce.Do(c.makeTVMClient)
}

func (c *Container) makeTVMClient() error {
	c.initializedTooling()
	if c.initFailed() {
		return c.initError
	}

	c.debug("initialize tvm client")
	cfg, err := c.GetTVMConfig()
	if err != nil {
		return err
	}

	c.debug("tvm api url", logf.Value(cfg.APIURL))
	c.debug("tvm client id", logf.Value(cfg.ClientID))

	c.tvmClient, err = tvmtool.NewClient(cfg.APIURL,
		tvmtool.WithSrc(cfg.ClientID),
		tvmtool.WithCacheEnabled(true),
		tvmtool.WithAuthToken(cfg.APIAuth),
		tvmtool.WithHTTPClient(&http.Client{Timeout: time.Millisecond * 500}),
		tvmtool.WithLogger(logging.Logger()),
	)
	if err != nil {
		return c.failInitF("tvm client init: %w", err)
	}
	c.debug("tvm client initialized")

	return nil
}
