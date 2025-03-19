package main

import (
	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/server"
)

func main() {
	pflag.Parse()
	cmdr := server.New()
	cmdr.Run()
}
