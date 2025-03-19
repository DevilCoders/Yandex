package main

import (
	"context"
	"fmt"

	iamoauth "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/oauth/v1"
	intapihttp "a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/internal-api/http"
	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/agentauth"
	proxyserver "a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/invertingproxy/server"
	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/server"
	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/userauth"
	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/util"
	as_grpc "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/compute/iam"
	iamgrpc "a.yandex-team.ru/cloud/mdb/internal/compute/iam/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

type Config struct {
	DisableAuth        bool                 `json:"disable_auth" yaml:"disable_auth"`
	ExposeErrorDetails bool                 `json:"expose_error_details" yaml:"expose_error_details"`
	UserAuth           userauth.Config      `json:"user_auth" yaml:"user_auth"`
	AuthorizedKey      AuthorizedKey        `json:"authorized_key" yaml:"authorized_key"`
	TokenService       TokenServiceConfig   `json:"token_service" yaml:"token_service"`
	SessionService     SessionServiceConfig `json:"session_service" yaml:"session_service"`
	AccessService      AccessServiceConfig  `json:"access_service" yaml:"access_service"`
	InternalAPI        intapihttp.Config    `json:"internal_api" yaml:"internal_api"`
	HTTPServer         server.Config        `json:"http_server" yaml:"http_server"`
	Proxy              proxyserver.Config   `json:"proxy" yaml:"proxy"`
}

func DefaultConfig() Config {
	return Config{
		DisableAuth:        false,
		ExposeErrorDetails: false,
		InternalAPI:        intapihttp.DefaultConfig(),
	}
}

type AuthorizedKey struct {
	ID               string `json:"id" yaml:"id"`
	ServiceAccountID string `json:"service_account_id" yaml:"service_account_id"`
	PrivateKey       string `json:"private_key" yaml:"private_key"`
}

type TokenServiceConfig struct {
	Addr     string                  `json:"addr" yaml:"addr"`
	Security grpcutil.SecurityConfig `json:"security" yaml:"security"`
}

type SessionServiceConfig struct {
	Addr     string                  `json:"addr" yaml:"addr"`
	Security grpcutil.SecurityConfig `json:"security" yaml:"security"`
}

type AccessServiceConfig struct {
	Addr     string                  `json:"addr" yaml:"addr"`
	Security grpcutil.SecurityConfig `json:"security" yaml:"security"`
}

const (
	ConfigName = "dataproc-ui-proxy.yaml"
)

func main() {
	cfg := DefaultConfig()
	if err := config.Load(ConfigName, &cfg); err != nil {
		fmt.Printf("failed to load application config, using defaults: %s", err)
		return
	}

	zapConfig := zap.KVConfig(log.DebugLevel)
	zapConfig.OutputPaths = []string{"stdout"}
	logger, err := zap.New(zapConfig)
	if err != nil {
		fmt.Printf("failed to initialize logger: %s", err)
		return
	}

	errorRenderer := util.NewErrorRenderer(cfg.ExposeErrorDetails, logger)

	var userAuth *userauth.UserAuth = nil
	var agentAuth *agentauth.AgentAuth = nil
	if !cfg.DisableAuth {
		tokenServiceClient, err := iamgrpc.NewTokenServiceClient(
			context.Background(),
			cfg.TokenService.Addr,
			"dataproc-ui-proxy",
			grpcutil.ClientConfig{
				Security: cfg.TokenService.Security,
			},
			&grpcutil.PerRPCCredentialsStatic{},
			logger,
		)
		if err != nil {
			logger.Fatalf("failed to initialize TokenService client: %s", err)
			return
		}

		key := cfg.AuthorizedKey
		sa := iam.ServiceAccount{
			ID:    key.ServiceAccountID,
			KeyID: key.ID,
			Token: []byte(key.PrivateKey),
		}
		credentials := tokenServiceClient.ServiceAccountCredentials(sa)

		conn, err := grpcutil.NewConn(
			context.Background(),
			cfg.SessionService.Addr,
			"dataproc-ui-proxy",
			grpcutil.ClientConfig{
				Security: cfg.SessionService.Security,
			},
			logger,
			grpcutil.WithClientCredentials(credentials),
		)
		if err != nil {
			logger.Fatalf("Failed to create SessionService connection: %s", err)
			return
		}
		sessionService := iamoauth.NewSessionServiceClient(conn)

		accessService, err := as_grpc.NewClient(
			context.Background(),
			cfg.AccessService.Addr,
			"dataproc-ui-proxy",
			grpcutil.ClientConfig{
				Security: cfg.AccessService.Security,
			},
			logger)
		if err != nil {
			logger.Fatalf("failed to initialize AccessService client: %s", err)
			return
		}
		internalAPI, err := intapihttp.New(cfg.InternalAPI, logger)
		if err != nil {
			logger.Fatalf("Failed to initialize intapi client: %s", err)
			return
		}
		userAuth, err = userauth.New(cfg.UserAuth, sessionService, internalAPI, accessService, errorRenderer, logger)
		if err != nil {
			logger.Fatalf("Failed to create user authenticator: %s", err)
			return
		}
		agentAuth = agentauth.New(internalAPI, accessService, errorRenderer, logger)
	}

	proxyServer := proxyserver.NewProxy(cfg.Proxy, logger)

	s := server.New(cfg.HTTPServer, agentAuth, userAuth, proxyServer, logger)
	s.Run()
}
