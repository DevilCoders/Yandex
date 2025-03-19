package main

import (
	"context"
	"os"
	"os/signal"
	"syscall"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	computedns "a.yandex-team.ru/cloud/mdb/internal/compute/dns"
	computegrpc "a.yandex-team.ru/cloud/mdb/internal/compute/dns/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/compute/iam"
	"a.yandex-team.ru/cloud/mdb/internal/compute/iam/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/metadb/pg"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	dnsconf "a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/config"
	dnscore "a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/core"
	"a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsapi"
	dac "a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsapi/compute"
	dnshttp "a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsapi/http"
	"a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsq"
	dqc "a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsq/compute"
	"a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsq/dnsal"
	"a.yandex-team.ru/library/go/core/log"
)

const (
	configPath = "/opt/yandex/mdb-dns/etc/"

	recTTL = 12 * time.Second
)

func createAndWaitMetadb(ctx context.Context, log log.Logger) metadb.MetaDB {
	var mdb metadb.MetaDB
	var cfgMetadb pgutil.Config
	err := config.LoadFromAbsolutePath(configPath+dnsconf.MetadbConfigName, &cfgMetadb)
	if err != nil {
		log.Fatalf("failed to prepare MetaDB endpoint: %s", err)
	}
	mdb, err = pg.New(cfgMetadb, log)
	if err != nil {
		log.Fatalf("failed to create pg MetaDB: %s", err)
	}
	log.Info("waiting for MetaDB...")
	tester := &ready.DefaultErrorTester{}
	err = ready.Wait(ctx, mdb, tester, time.Second)
	if err != nil {
		log.Fatalf("failed to wait MetaDB ready: %s", err)
	}
	log.Info("MetaDB ready...")
	return mdb
}

// get reduced version of DAClient
func createCommon(ctx context.Context, log log.Logger, cfgMdbdns dnsconf.Config, env *cli.Env, useCompute bool) (context.Context, *dnscore.DAClient, computedns.Client) {
	configPathToLoadFrom := env.GetConfigPath()
	if configPathToLoadFrom == "" {
		configPathToLoadFrom = configPath
	}
	err := config.LoadFromAbsolutePath(configPathToLoadFrom+dnsconf.MdbdnsConfigName, &cfgMdbdns)
	if err != nil {
		log.Errorf("failed to load application config, using defaults: %s", err)
	}
	apiCfg := cfgMdbdns.DNSAPI.Slayer
	if useCompute {
		apiCfg = cfgMdbdns.DNSAPI.Compute
	}

	var compute computedns.Client
	ctx, cancel := context.WithCancel(ctx)
	signals := make(chan os.Signal)
	signal.Notify(signals, os.Interrupt, syscall.SIGTERM)
	go func() {
		<-signals
		cancel()
	}()

	log.Infof("resolve over %s", apiCfg.ResolveNS)

	httpCfg := httputil.DefaultTransportConfig()
	httpCfg.TLS.CAFile = cfgMdbdns.DMConf.CertPath
	rt, err := httputil.NewTransport(httpCfg, log)
	if err != nil {
		log.Fatalf("failed to create new HTTP transport: %s", err)
	}

	var dq dnsq.Client
	var da dnsapi.Client
	api := dnscore.DNSSlayer
	if useCompute {
		api = dnscore.DNSCompute
		envCfg := cfgMdbdns.Environment
		client, err := grpc.NewTokenServiceClient(
			ctx,
			envCfg.Services.Iam.V1.TokenService.Endpoint,
			cfgMdbdns.AppName,
			grpcutil.DefaultClientConfig(),
			&grpcutil.PerRPCCredentialsStatic{},
			log,
		)
		if err != nil {
			log.Fatalf("failed to initialize token service client %v", err)
		}
		creds := client.ServiceAccountCredentials(iam.ServiceAccount{
			ID:    cfgMdbdns.ServiceAccount.ID,
			KeyID: cfgMdbdns.ServiceAccount.KeyID.Unmask(),
			Token: []byte(cfgMdbdns.ServiceAccount.PrivateKey.Unmask()),
		})
		compute, err = computegrpc.New(ctx, log, apiCfg.GRPCurl, cfgMdbdns.AppName, apiCfg.GRPCCfg, creds, uint(recTTL.Seconds()))
		if err != nil {
			log.Fatalf("failed to create VPC DNS API client: %s", err)
		}
		da = dac.New(compute)
		dq = dqc.New(log, compute, 2*recTTL)
	} else {
		dq = dnsal.New(log, apiCfg.ResolveNS)
		da = dnshttp.New(log, rt, apiCfg, recTTL)
	}

	// reduced vesion of original DAClient
	dac := &dnscore.DAClient{
		Client: api,
		Suffix: apiCfg.FQDNSuffix,
		MaxRec: apiCfg.MaxRec,
		UpdThr: apiCfg.UpdThrs,
		DQ:     dq,
		DA:     da,
	}

	return ctx, dac, compute
}
