package main

import (
	"context"
	"fmt"
	"os"
	"strings"
	"time"

	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/cloud/mdb/internal/nacl"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/cmd/mdb-secrets-secrets-import/certs"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb/pg"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

// flags
var (
	srcConfigFlagValue         = pflag.String("src-conf", internal.ConfigName, "path to mdb-secrets config file")
	dstConfigFlagValue         = pflag.String("dst-conf", internal.ConfigName, "path to mdb-secrets config file")
	srcPublicKeyFlagValue      = pflag.String("src-public-key", "", "secrets public key. Find it on salt-master (pgcerts.public_key in /etc/salt/ext_pillars)")
	srcSaltPrivateKeyFlagValue = pflag.String("src-salt-private-key", "", "salt private key. Find it on salt-master (pgcerts.salt_private_key in /etc/salt/ext_pillars)")
	dryRun                     = pflag.Bool("dry-run", true, "don't import and modify anything, just read")
	clusterID                  = pflag.String("cid", "", "cluster ids, separated by comma, required if importing gpg")
	hostname                   = pflag.String("hostname", "", "hostnames, separated by comma, required if importing certs")
	mode                       = pflag.String("mode", "gpg", "gpg or crt")
)

const (
	dbTimeout = 10 * time.Second
)

// Import certificates and gpg keys from secrets to secrets
func main() {
	pflag.Parse()

	//config
	var srcCfg internal.Config
	if err := config.Load(*srcConfigFlagValue, &srcCfg); err != nil {
		fmt.Printf("failed to load application config: %s\n", err.Error())
		os.Exit(1)
	}

	var dstCfg internal.Config
	if err := config.Load(*dstConfigFlagValue, &dstCfg); err != nil {
		fmt.Printf("failed to load application config: %s\n", err.Error())
		os.Exit(1)
	}

	//logger
	logger, err := zap.New(zap.CLIConfig(srcCfg.App.Logging.Level))
	if err != nil {
		fmt.Printf("failed to initialize logger: %s\n", err.Error())
		os.Exit(1)
	}

	dstSaltPublicKey, err := nacl.ParseKey(dstCfg.SaltPublicKey)
	if err != nil {
		logger.Fatalf("invalid dst salt public key: %s", err.Error())
	}
	dstSecretsPrivateKey, err := nacl.ParseKey(dstCfg.PrivateKey.Unmask())
	if err != nil {
		logger.Fatalf("invalid dst secrets private key: %s", err.Error())
	}

	srcSecretsPublicKey, err := nacl.ParseKey(*srcPublicKeyFlagValue)
	if err != nil {
		logger.Fatalf("invalid src secrets public key: %s", err.Error())
	}
	srcSaltPrivateKey, err := nacl.ParseKey(*srcSaltPrivateKeyFlagValue)
	if err != nil {
		logger.Fatalf("invalid salt private key: %s", err.Error())
	}

	srcSaltPublicKey, err := nacl.ParseKey(srcCfg.SaltPublicKey)
	if err != nil {
		logger.Fatalf("invalid src salt public key: %s", err.Error())
	}

	srcSecretsPrivateKey, err := nacl.ParseKey(srcCfg.PrivateKey.Unmask())
	if err != nil {
		logger.Fatalf("invalid src secrets private key: %s", err.Error())
	}

	srcSecrets, err := pg.New(srcCfg.SecretsDB, logger, srcSaltPublicKey, srcSecretsPrivateKey)
	if err != nil {
		logger.Fatalf("init srcSecrets db: %s", err.Error())
	}

	dstSecrets, err := pg.New(dstCfg.SecretsDB, logger, dstSaltPublicKey, dstSecretsPrivateKey)
	if err != nil {
		logger.Fatalf("init dstSecrets db: %s", err.Error())
	}

	//wait for secret DB
	time.Sleep(2 * time.Second)
	err = waitForDB("src secrets db", logger, srcSecrets)
	if err != nil {
		logger.Fatalf("db readiness fail: %s", err.Error())
	}
	err = waitForDB("dst secrets db", logger, dstSecrets)
	if err != nil {
		logger.Fatalf("db readiness fail: %s", err)
	}

	pr := &certs.Processor{
		SrcSecrets:          srcSecrets,
		DstSecrets:          dstSecrets,
		L:                   logger,
		SrcSecretsPublicKey: srcSecretsPublicKey,
		SrcSaltPrivateKey:   srcSaltPrivateKey,
	}

	ctx := context.Background()

	switch *mode {
	case "gpg":
		if *clusterID == "" {
			logger.Fatalf("cid should be specified in %s mode", *mode)
		}
		cids := strings.Split(*clusterID, ",")
		for _, cid := range cids {
			cid = strings.TrimSpace(cid)
			if err := pr.CopyGpg(ctx, cid, *dryRun); err != nil {
				logger.Fatalf("%s: failed to import gpg: %s", cid, err.Error())
			}
		}
	case "crt":
		if *hostname == "" {
			logger.Fatalf("hostname should be specified in %s mode", *mode)
		}
		hostnames := strings.Split(*hostname, ",")
		for _, hostname := range hostnames {
			hostname = strings.TrimSpace(hostname)
			if err := pr.CopyCrt(ctx, hostname, *dryRun); err != nil {
				logger.Fatalf("%s: failed to import crt: %s", hostname, err.Error())
			}
		}
	default:
		logger.Fatalf("unknown mode %q", *mode)
	}

	//logger.Infof("Import finished.\nDry run: %v\nMode: %s\nCluster ID: %s\nHostname: %s", *dryRun, *mode, *clusterID, *hostname)
}

func waitForDB(name string, logger log.Logger, rc ready.Checker) error {
	return ready.WaitWithTimeout(
		context.Background(),
		dbTimeout,
		rc,
		&ready.DefaultErrorTester{Name: name, L: logger},
		time.Second)
}
