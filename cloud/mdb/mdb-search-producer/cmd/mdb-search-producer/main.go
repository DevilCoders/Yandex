package main

import (
	"os"

	"a.yandex-team.ru/cloud/mdb/mdb-search-producer/internal/app"
)

func main() {
	os.Exit(app.ConfigureAndRun())
}
