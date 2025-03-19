package main

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/app/signals"
	"a.yandex-team.ru/cloud/mdb/mdb-disklock/internal/app"
)

func main() {
	ctx := signals.WithCancelOnSignal(context.Background())
	app.Run(ctx)
}
