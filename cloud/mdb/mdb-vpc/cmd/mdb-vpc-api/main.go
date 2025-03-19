package main

import (
	"fmt"
	"os"

	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/api"
)

func main() {
	app, err := api.New()
	if err != nil {
		fmt.Printf("failed create app: %v\n", err)
		os.Exit(1)
	}

	app.Run()
}
