package main

import (
	"fmt"
	"os"

	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/cloud/mdb/mlock/internal/server"
)

func main() {
	if len(os.Args) != 2 {
		fmt.Fprintf(os.Stderr, "Usage %s <config file>\n", os.Args[0])
		os.Exit(1)
	}

	conf := server.DefaultConfig()

	err := config.LoadFromAbsolutePath(os.Args[1], &conf)

	if err != nil {
		panic(err)
	}

	app, err := server.NewApp(conf)

	if err != nil {
		panic(err)
	}

	err = app.Run()

	if err != nil {
		panic(err)
	}
}
