package main

import (
	"fmt"
	"os"

	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/worker"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/worker/config"
)

func main() {
	cfg := config.DefaultConfig()
	options := app.DefaultServiceOptions(&cfg, fmt.Sprintf("%s.yaml", config.AppCfgName))
	baseApp, err := app.New(options...)
	if err != nil {
		fmt.Printf("cannot make base app, %s\n", err)
		os.Exit(1)
	}
	config.LoadSecrets(&cfg, baseApp.L())

	logger := baseApp.L()

	w := worker.NewWorkerFromConfig(baseApp.ShutdownContext(), logger, cfg)

	go baseApp.WaitForStop()
	w.Run(baseApp.ShutdownContext())
}
