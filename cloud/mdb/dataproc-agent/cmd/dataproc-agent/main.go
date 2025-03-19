package main

import (
	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/agent"
)

func main() {
	pflag.Parse()
	cmdr := agent.New()
	cmdr.Run()
}
