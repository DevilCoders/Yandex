package internal

import (
	"fmt"
	"net/http"
	"os"

	"github.com/go-openapi/runtime/middleware"
	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/app/swagger"
	computedns "a.yandex-team.ru/cloud/mdb/internal/compute/dns"
	grpccompute "a.yandex-team.ru/cloud/mdb/internal/compute/dns/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/compute/iam"
	"a.yandex-team.ru/cloud/mdb/internal/compute/iam/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/flags"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/metadb/pg"
	"a.yandex-team.ru/cloud/mdb/internal/prometheus"
	dnsconf "a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/config"
	"a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/core"
	dac "a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsapi/compute"
	das "a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsapi/http"
	dar "a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsapi/route53"
	dqc "a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsq/compute"
	dqs "a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsq/dnsal"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

// App main application object - handles setup and teardown
type App struct {
	*swagger.App

	dnsm *core.DNSManager
	cfg  dnsconf.Config

	mdwCtx *middleware.Context
}

const (
	flagKeyLogLevel    = "mdb-loglevel"
	flagKeyLogHTTPBody = "mdb-loghttpbody"

	// https://docs.aws.amazon.com/vpc/latest/userguide/VPC_DHCP_Options.html#AmazonDNS
	amazonDNSServer = "169.254.169.253"
)

var appFlags *pflag.FlagSet

func init() {
	defcfg := dnsconf.DefaultConfig()
	appFlags = pflag.NewFlagSet("App", pflag.ExitOnError)
	appFlags.String(flagKeyLogLevel, defcfg.LogLevel.String(), "Set log level")
	appFlags.Bool(flagKeyLogHTTPBody, defcfg.LogHTTPBody, "Should we log body of each HTTP request")
	pflag.CommandLine.AddFlagSet(appFlags)
	flags.RegisterConfigPathFlagGlobal()
}

// NewApp constructs application object
// nolint: gocyclo
func NewApp(mdwCtx *middleware.Context) *App {
	// Default logger so we can log something even when we have no configuration loaded
	logger, err := zap.New(zap.KVConfig(log.DebugLevel))
	if err != nil {
		fmt.Printf("failed to initialize logger: %s\n", err)
		os.Exit(1)
	}

	cfg := dnsconf.DefaultConfig()
	if appFlags.Changed(flagKeyLogLevel) {
		var ll string
		if ll, err = appFlags.GetString(flagKeyLogLevel); err == nil {
			cfg.LogLevel, err = log.ParseLevel(ll)
			if err != nil {
				logger.Fatalf("failed to parse loglevel: %s", err)
			}
		}
	}

	baseApp, err := swagger.New(mdwCtx, app.DefaultServiceOptions(&cfg, dnsconf.MdbdnsConfigName)...)
	if err != nil {
		logger.Fatalf("initialize base app %s", err)
	}

	pgConf := pg.LoadConfig(baseApp.L(), dnsconf.MetadbConfigName)
	mdb, err := pg.New(pgConf, baseApp.L())
	if err != nil {
		baseApp.L().Fatalf("failed to prepare MetaDB endpoint: %s", err)
	}

	cdm := cfg.DMConf
	var daList []*core.DAClient
	if cdm.DAUse.Slayer {
		cda := cfg.DNSAPI.Slayer

		rt, err := httputil.DEPRECATEDNewTransport(
			httputil.TLSConfig{CAFile: cdm.CertPath},
			httputil.LoggingConfig{LogRequestBody: cda.LogHTTPBody, LogResponseBody: cda.LogHTTPBody},
			baseApp.L(),
		)
		if err != nil {
			baseApp.L().Fatalf("failed to create transport: %s", err)
		}

		da := das.New(baseApp.L(), rt, cda, cdm.DNSTTL)
		dq := dqs.New(baseApp.L(), cda.ResolveNS)
		daList = append(daList, &core.DAClient{
			Client: core.DNSSlayer,
			DA:     da,
			DQ:     dq,
			Suffix: cda.FQDNSuffix,
			MaxRec: cda.MaxRec,
			UpdThr: cda.UpdThrs,
		})
	}

	ctx := baseApp.ShutdownContext()
	if cdm.DAUse.Compute {
		cda := cfg.DNSAPI.Compute

		envCfg := cfg.Environment
		client, err := grpc.NewTokenServiceClient(
			ctx,
			envCfg.Services.Iam.V1.TokenService.Endpoint,
			cfg.AppName,
			grpcutil.DefaultClientConfig(),
			&grpcutil.PerRPCCredentialsStatic{},
			baseApp.L(),
		)
		if err != nil {
			baseApp.L().Fatalf("failed to initialize token service client %v", err)
		}
		creds := client.ServiceAccountCredentials(iam.ServiceAccount{
			ID:    cfg.ServiceAccount.ID,
			KeyID: cfg.ServiceAccount.KeyID.Unmask(),
			Token: []byte(cfg.ServiceAccount.PrivateKey.Unmask()),
		})

		var dc computedns.Client

		dc, err = grpccompute.New(ctx, baseApp.L(), cda.GRPCurl, "mdb-dns", cda.GRPCCfg, creds, uint(cdm.DNSTTL.Seconds()))
		if err != nil {
			baseApp.L().Fatalf("failed to create VPC DNS API client: %s", err)
		}

		da := dac.New(dc)
		dq := dqc.New(baseApp.L(), dc, 2*cdm.DNSTTL)
		daList = append(daList, &core.DAClient{
			Client: core.DNSCompute,
			DA:     da,
			DQ:     dq,
			Suffix: cda.FQDNSuffix,
			MaxRec: cda.MaxRec,
			UpdThr: cda.UpdThrs,
		})
	}
	if cdm.DAUse.Route53 {
		cdr := cfg.DNSAPI.Route53

		da := dar.New(logger, cdr, uint64(cdm.DNSTTL.Seconds()))
		dq := dqs.New(baseApp.L(), amazonDNSServer)
		daList = append(daList, &core.DAClient{
			Client: core.DNSRoute53,
			DA:     da,
			DQ:     dq,
			Suffix: cdr.FQDNSuffix,
			MaxRec: cdr.MaxRec,
			UpdThr: cdr.UpdThrs,
		})

		if cdr.PrivateFQDNSuffix != "" {
			daList = append(daList, &core.DAClient{
				Client: core.DNSRoute53,
				DA:     da,
				DQ:     dq,
				Suffix: cdr.PrivateFQDNSuffix,
				PubSuf: cdr.FQDNSuffix,
				MaxRec: cdr.MaxRec,
				UpdThr: cdr.UpdThrs,
			})
		}
	}
	if len(daList) == 0 {
		baseApp.L().Fatalf("no active dns api client selected")
	}

	return &App{
		App:    baseApp,
		dnsm:   core.NewDNSManager(ctx, baseApp.L(), cdm, daList, mdb),
		cfg:    cfg,
		mdwCtx: mdwCtx,
	}
}

// DNSManager accessor
func (app *App) DNSManager() *core.DNSManager {
	return app.dnsm
}

// SetupGlobalMiddleware setups global middleware
func (app *App) SetupGlobalMiddleware(next http.Handler) http.Handler {
	return httputil.LoggingMiddleware(
		httputil.RequestBodySavingMiddleware(
			httputil.RequestIDMiddleware(
				prometheus.Middleware(
					httputil.TracingSwaggerMiddleware(next, app.cfg.App.Tracing.ServiceName, app.MiddlewareContext()),
					app.L()),
			),
			app.L(),
		),
		httputil.LoggingConfig{
			LogRequestBody:  app.cfg.LogHTTPBody,
			LogResponseBody: app.cfg.LogHTTPBody,
		},
		app.L(),
	)
}
