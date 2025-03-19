package recipe

import (
	"fmt"
	"os"

	metadbhelpers "a.yandex-team.ru/cloud/mdb/dbaas_metadb/recipes/helpers"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/godogutil"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/internal/testutil"
	pillarapp "a.yandex-team.ru/cloud/mdb/mdb-pillar-config/internal"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func StartPillarConfigAPI() error {
	const tries = 3
	var errs []error

	for i := 0; i < tries; i++ {
		a, port, err := newApp()
		if err != nil {
			fmt.Printf("failed to initialize internal api: %s\n", err)
			errs = append(errs, err)
			continue
		}

		fmt.Printf("Using %s port for MDB internal API\n", port)
		go func() {
			a.WaitForStop()
		}()

		if err := os.Setenv("MDB_PILLAR_CONFIG_PORT", port); err != nil {
			return err
		}

		return nil
	}

	// TODO: use multierr when available
	return xerrors.Errorf("failed to initialize mdb-pillar-config with random port %d times: %+v", tries, errs)
}

func newApp() (*pillarapp.App, string, error) {
	logPath := godogutil.TestOutputPath("logs/mdb-pillar-config.log")

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

	cfg := pillarapp.DefaultConfig()
	cfg.App.Logging.Level = log.DebugLevel
	cfg.App.Logging.File = logPath
	cfg.API.ExposeErrorDebug = true
	cfg.HTTP.Addr = "[::1]:" + port
	cfg.HTTP.Logging = httputil.LoggingConfig{
		LogRequestBody:  true,
		LogResponseBody: true,
	}
	cfg.MetaDB = pgutil.Config{
		Addrs:   []string{metaDBHost + ":" + metaDBPort},
		SSLMode: "disable",
		User:    "dbaas_api",
		DB:      "dbaas_metadb",
	}

	baseApp, err := app.New(
		app.WithConfig(&cfg),
		app.WithLoggerConstructor(app.DefaultToolLoggerConstructor()),
	)

	if err != nil {
		return nil, "", err
	}

	a, err := pillarapp.NewApp(baseApp, cfg, pillarapp.AppComponents{})

	return a, port, err
}
