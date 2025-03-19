package main

import (
	"os"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/reindexer"
)

func main() {
	os.Exit(reindexer.Run())
}
