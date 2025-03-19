package internal

import "a.yandex-team.ru/cloud/mdb/internal/app"

const configFilename = "mdb-reaper.yaml"

func Run() {
	cfg := DefaultConfig()
	if !cfg.App.ServiceAccount.FromEnv("REAPER") {
		panic("REAPER_PRIVATE_KEY is empty")
	}
	baseOpts := app.DefaultCronOptions(&cfg, configFilename)
	baseApp, err := app.New(baseOpts...)
	if err != nil {
		panic(err)
	}
	l := baseApp.L()
	if !cfg.MetaDB.Password.FromEnv("METADB_PASSWORD") {
		l.Fatal("METADB_PASSWORD is empty")
	}
	a, err := NewApp(baseApp, WithConfig(cfg))
	if err != nil {
		baseApp.L().Fatalf("app creation error: %v", err)
	}
	defer a.Shutdown()
	a.L().Info("reaper tool is created")

	err = a.Reap()
	if err != nil {
		a.L().Fatalf("Reaping failed: %s", err)
	}
}
