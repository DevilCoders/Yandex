package main

import (
	"fmt"
	"os"

	"a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/cmds"
)

func main() {
	env, err := cmds.New("mdb-porto-agent")
	if err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
	if err = env.RootCmd.Cmd.Execute(); err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
}
