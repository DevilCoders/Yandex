package main

import (
	"bufio"
	"context"
	"fmt"
	"io"
	"os"
	"time"

	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/cloud/mdb/internal/nacl"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/cmd/mdb-secrets-import/certs"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/cmd/mdb-secrets-import/gpg"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb/pg"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

// flags
var (
	configFlagValue = pflag.String("conf", internal.ConfigName, "path to mdb-secrets config file")
	modeFlagValue   = pflag.String("mode", modeGpg, "import mode: gpg or cert")
)

const (
	modeGpg  = "gpg"
	modeCert = "cert"

	dbTimeout = 10 * time.Second
)

type LineProcessor interface {
	ProcessLine(string) error
}

var secrets secretsdb.Service

//accepts filenames from stdin and extracts gpg private keys from them
//imports those keys in secretsdb with cid (cluster id) as filename without extension ( /path/to/test01.gpg -> test01)
func main() {
	pflag.Parse()

	var lineProcessor LineProcessor

	//mode
	switch *modeFlagValue {
	case modeGpg:
		fmt.Println("gpg mode")
	case modeCert:
		fmt.Println("certs mode")
	default:
		fmt.Println("incorrect 'mode' key value")
		pflag.Usage()
		os.Exit(1)
	}

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

	secrets, err = pg.New(cfg.SecretsDB, logger, saltPubKey, privateKey)
	if err != nil {
		logger.Fatalf("init secrets DB: %s", err.Error())
	}

	//wait for db
	err = waitForDB(logger)
	if err != nil {
		logger.Fatalf("db readiness fail: %s", err.Error())
	}

	switch *modeFlagValue {
	case modeGpg:
		lineProcessor = &gpg.Processor{
			Secrets: secrets,
		}
	case modeCert:
		lineProcessor = &certs.Processor{
			Secrets: secrets,
		}
	}

	r := bufio.NewReader(os.Stdin)

	var success, failed, skipped int
TOP:
	for i := 1; ; i++ {
		lineB, isPrefix, err := r.ReadLine()
		switch {
		case err == io.EOF:
			break TOP
		case err != nil:
			logger.Errorf("error reading line #%d: %s:", i, err.Error())
			continue
		}
		if isPrefix {
			continue
		}

		line := string(lineB)
		pErr := lineProcessor.ProcessLine(line)
		switch {
		case pErr == nil:
			logger.Infof("success: %s", line)
			success++
		default:
			logger.Infof("error: %s", pErr.Error())
			failed++
		}
	}

	logger.Infof("Import finished. Result:\nSuccess: %d\nFailed:  %d\nTotal:   %d", success, failed, success+skipped+failed)
}

func waitForDB(logger log.Logger) error {
	ctx, cancel := context.WithTimeout(context.Background(), dbTimeout)
	defer cancel()

	for {
		err := secrets.IsReady(ctx)
		if err != nil {
			logger.Debugf("db is not ready: %s", err.Error())
		} else {
			break
		}
		select {
		case <-ctx.Done():
			return fmt.Errorf("db readiness timeout (%v) exceeded", dbTimeout)
		default:
			time.Sleep(1 * time.Second)
			continue
		}
	}
	return nil
}
