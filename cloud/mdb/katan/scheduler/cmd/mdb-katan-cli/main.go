package main

import (
	"fmt"
	"os"
	"os/user"

	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/katan/scheduler/pkg/app"
)

var (
	commands, tags, config, userName string
)

func init() {
	pflag.StringVar(&config, "config", app.ConfigPath, "path to config file")
	pflag.StringVar(&commands, "commands", "", "deploy commands")
	pflag.StringVar(&tags, "tags", "", "tags define rollout clusters")
	pflag.StringVar(&userName, "user", "", "your name")
}

func main() {
	pflag.Parse()

	if len(userName) == 0 {
		runAs, _ := user.Current()
		if runAs == nil {
			fmt.Println("unable to define user name. Pass it with --user option.")
			os.Exit(1)
		}
		userName = runAs.Username
	}

	os.Exit(app.Cli(config, tags, commands, userName))
}
