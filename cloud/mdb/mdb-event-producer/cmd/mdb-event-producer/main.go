package main

import (
	"os"

	"a.yandex-team.ru/cloud/mdb/mdb-event-producer/internal/app"
)

func main() {
	os.Exit(app.ConfigureAndRun())
}
