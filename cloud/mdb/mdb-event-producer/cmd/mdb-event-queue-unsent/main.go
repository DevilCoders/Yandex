package main

import (
	"a.yandex-team.ru/cloud/mdb/mdb-event-producer/internal/monitoring"
)

func main() {
	monitoring.CheckEventsAge()
}
