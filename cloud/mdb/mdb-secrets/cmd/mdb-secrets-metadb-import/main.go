package main

import (
	"context"
	"fmt"
	"os"
	"time"

	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/cloud/mdb/internal/nacl"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/cmd/mdb-secrets-metadb-import/internal/certs"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/cmd/mdb-secrets-metadb-import/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb/pg"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

// flags
var (
	configFlagValue               = pflag.String("conf", internal.ConfigName, "path to mdb-secrets config file")
	metaAddrFlagValue             = pflag.String("metadb-addr", "", "metadb address in style: meta01h.db.yandex.net:6432")
	metaUserFlagValue             = pflag.String("metadb-user", "dbaas_api", "metadb user")
	metaPasswordFlagValue         = pflag.String("metadb-password", "", "metadb password for given user")
	internalAPIPublicKeyFlagValue = pflag.String("internal-api-public-key", "", "internal API public key. Find it on salt-master (dbaas_pillar.api_pub_key in /etc/salt/ext_pillars)")
	saltPrivateKeyFlagValue       = pflag.String("salt-private-key", "", "salt private key. Find it on salt-master (dbaas_pillar.salt_sec_key in /etc/salt/ext_pillars)")
	dryRun                        = pflag.Bool("dry-run", true, "don't import and modify anything, just read")
	ca                            = pflag.String("ca", "InternalCA", "InternalCA for porto and YcInternalCA for compute")
	clusterID                     = pflag.String("cid", "", "cluster id, apply to all if not specified ")
)

const (
	dbTimeout = 10 * time.Second
)

func formMetaConfig() pgutil.Config {
	metaCfg := metadb.DefaultConfig()
	metaCfg.Addrs = []string{*metaAddrFlagValue}
	metaCfg.DB = "dbaas_metadb"
	metaCfg.User = *metaUserFlagValue
	metaCfg.Password = secret.NewString(*metaPasswordFlagValue)
	return metaCfg
}

// Import certificates from metadb to secrets
func main() {
	pflag.Parse()

	//config
	var cfg internal.Config
	if err := config.Load(*configFlagValue, &cfg); err != nil {
		fmt.Printf("failed to load application config: %s\n", err.Error())
		os.Exit(1)
	}

	//logger
	logger, err := zap.New(zap.CLIConfig(cfg.App.Logging.Level))
	if err != nil {
		fmt.Printf("failed to initialize logger: %s\n", err.Error())
		os.Exit(1)
	}

	saltPubKey, err := nacl.ParseKey(cfg.SaltPublicKey)
	if err != nil {
		logger.Fatalf("invalid salt public key: %s", err.Error())
	}
	privateKey, err := nacl.ParseKey(cfg.PrivateKey.Unmask())
	if err != nil {
		logger.Fatalf("invalid mdb-secrets private key: %s", err.Error())
	}
	internalAPIPublicKey, err := nacl.ParseKey(*internalAPIPublicKeyFlagValue)
	if err != nil {
		logger.Fatalf("invalid internal-api public key: %s", err.Error())
	}
	saltPrivateKey, err := nacl.ParseKey(*saltPrivateKeyFlagValue)
	if err != nil {
		logger.Fatalf("invalid salt private key: %s", err.Error())
	}

	secrets, err := pg.New(cfg.SecretsDB, logger, saltPubKey, privateKey)
	if err != nil {
		logger.Fatalf("init secrets db: %s", err.Error())
	}

	mdb, err := metadb.New(formMetaConfig(), logger)
	if err != nil {
		logger.Fatalf("initialize metadb: %s", err)
	}

	//wait for secret DB
	time.Sleep(2 * time.Second)
	err = waitForDB("secrets DB", logger, secrets)
	if err != nil {
		logger.Fatalf("db readiness fail: %s", err.Error())
	}
	err = waitForDB("metadb", logger, mdb)
	if err != nil {
		logger.Fatalf("metadb readiness fail: %s", err)
	}

	pr := &certs.Processor{Secrets: secrets, L: logger, InternalAPIPublicKey: internalAPIPublicKey, SaltPrivateKey: saltPrivateKey}

	ctx := context.Background()

	var metaCerts []metadb.HostCerts
	if pflag.CommandLine.Changed("cid") {
		metaCerts, err = mdb.GetClusterCertificates(ctx, *clusterID)
	} else {
		metaCerts, err = mdb.GetHostsCertificates(ctx)
	}
	if err != nil {
		logger.Fatalf("get certs from metadb: %s", err)
	}

	logger.Infof("got %d certs from metadb", len(metaCerts))

	var success, failed int
	for _, crt := range metaCerts {
		if err := pr.Process(ctx, crt.FQDN, *ca, crt.EncryptedCrt, crt.EncryptedKey, *dryRun); err != nil {
			failed++
			logger.Errorf("%s failed: %s", crt.FQDN, err)
			continue
		}
		success++
		if !(*dryRun) {
			if err := mdb.RemoveCertificates(ctx, crt.ClusterID, crt.FQDN); err != nil {
				logger.Errorf("%s failed to remove certificate from metadb: %s", crt.FQDN, err)
			}
		}
		logger.Infof("%s succeeded", crt.FQDN)
	}

	logger.Infof("Import finished.\nDry run: %v\nSucceeded: %d\nFailed: %d", *dryRun, success, failed)
}

func waitForDB(name string, logger log.Logger, rc ready.Checker) error {
	return ready.WaitWithTimeout(
		context.Background(),
		dbTimeout,
		rc,
		&ready.DefaultErrorTester{Name: name, L: logger},
		time.Second)
}
