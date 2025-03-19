package main

import (
	"a.yandex-team.ru/cloud/mdb/mdb-search-producer/internal/monitoring"
)

func main() {
	monitoring.CheckSearchQueueDocumentsAge()
}
