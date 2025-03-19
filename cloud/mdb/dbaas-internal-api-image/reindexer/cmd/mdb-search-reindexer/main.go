package main

import (
	"os"

	"a.yandex-team.ru/cloud/mdb/dbaas-internal-api-image/reindexer/internal/reindexer"
)

func main() {
	os.Exit(reindexer.Run())
}
