package main

import (
	"os"

	"a.yandex-team.ru/cloud/infra/mdbcmsapiclient/internal/cli"
)

var version string = "0.1.0-alpha"

func main() {
	if err := cli.Execute(version); err != nil {
		os.Exit(1)
	}
}
