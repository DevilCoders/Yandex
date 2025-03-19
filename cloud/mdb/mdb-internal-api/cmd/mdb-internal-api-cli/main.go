package main

import (
	"fmt"
	"os"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/cli"
)

func main() {
	rootCmd := cli.New("mdb-internal-api-cli")
	if err := rootCmd.Execute(); err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
}
