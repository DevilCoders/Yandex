package main

import (
	"log"
	"os"

	internal "a.yandex-team.ru/cloud/mdb/windows/mssync/internal/app"
)

func main() {
	app, err := internal.NewApp()
	if err != nil {
		log.Fatalf("Failed to create mssync app: %v", err)
	}
	app.Run()
	os.Exit(1)
}
