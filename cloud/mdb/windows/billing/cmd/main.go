package main

import (
	"log"

	"a.yandex-team.ru/cloud/mdb/windows/billing/internal"
)

func main() {
	app, err := internal.NewApp()
	if err != nil {
		log.Fatalf("Failed to create billing app: %v", err)
	}
	app.Run(app.ShutdownContext())
}
