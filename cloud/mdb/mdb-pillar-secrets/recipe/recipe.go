package recipe

import (
	"fmt"
	"os"

	metadbhelpers "a.yandex-team.ru/cloud/mdb/dbaas_metadb/recipes/helpers"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	"a.yandex-team.ru/cloud/mdb/internal/godogutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/interceptors"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/internal/testutil"
	secretsapp "a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/internal"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func StartPillarSecretsAPI(as as.AccessService) error {
	const tries = 3
	var errs []error

	for i := 0; i < tries; i++ {
		a, port, err := newApp(as)
		if err != nil {
			fmt.Printf("failed to initialize internal api: %s\n", err)
			errs = append(errs, err)
			continue
		}

		fmt.Printf("Using %s port for MDB internal API\n", port)
		go func() {
			a.WaitForStop()
		}()

		if err := os.Setenv("MDB_PILLAR_SECRETS_PORT", port); err != nil {
			return err
		}

		return nil
	}

	// TODO: use multierr when available
	return xerrors.Errorf("failed to initialize mdb-pillar-config with random port %d times: %+v", tries, errs)
}

func newApp(as as.AccessService) (*secretsapp.App, string, error) {
	logPath := godogutil.TestOutputPath("logs/mdb-pillar-secrets.log")

	port, err := testutil.GetFreePort()
	if err != nil {
		return nil, "", xerrors.Errorf("failed to get free port: %w", err)
	}

	metaDBHost, err := metadbhelpers.Host()
	if err != nil {
		return nil, "", err
	}
	metaDBPort, err := metadbhelpers.Port()
	if err != nil {
		return nil, "", err
	}

	cfg := secretsapp.DefaultConfig()
	cfg.App.Logging.Level = log.DebugLevel
	cfg.App.Logging.File = logPath
	cfg.API.ExposeErrorDebug = true
	cfg.GRPC.Addr = "[::1]:" + port
	cfg.GRPC.Logging = interceptors.LoggingConfig{
		LogRequestBody:  true,
		LogResponseBody: true,
	}
	cfg.MetaDB = pgutil.Config{
		Addrs:   []string{metaDBHost + ":" + metaDBPort},
		SSLMode: "disable",
		User:    "dbaas_api",
		DB:      "dbaas_metadb",
	}

	cfg.Crypto.PeersPublicKey = "0000000000000000000000000000000000000000000="
	cfg.Crypto.PrivateKey = secret.NewString("0000000000000000000000000000000000000000000=")

	baseApp, err := app.New(
		app.WithConfig(&cfg),
		app.WithLoggerConstructor(app.DefaultToolLoggerConstructor()),
	)
	if err != nil {
		return nil, "", err
	}

	a, err := secretsapp.NewApp(baseApp, cfg, secretsapp.AppComponents{
		AccessService: as,
	})

	return a, port, err
}
