package main

import (
	"fmt"
	"os"

	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/agentapp"
)

func main() {
	a, err := agentapp.New()
	if err != nil {
		fmt.Printf("app init fails unexpectedly: %s", err)
		os.Exit(1)
	}
	a.Run()
}
