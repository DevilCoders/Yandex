package main

import (
	"context"
	"fmt"
	"os"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/grpcserver"
)

func main() {
	ctx := context.Background()
	app, err := grpcserver.New(ctx)
	if err != nil {
		fmt.Printf("failed create app: %v\n", err)
		os.Exit(1)
	}

	app.Run()
}
