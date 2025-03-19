package main

import (
	"os"

	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/katan/scheduler/pkg/app"
)

var (
	config string
)

func init() {
	pflag.StringVar(&config, "config", app.ConfigPath, "path to config file")
}

func main() {
	pflag.Parse()
	os.Exit(app.ProcessSchedules(config))
}
