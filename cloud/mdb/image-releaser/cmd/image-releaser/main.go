package main

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/image-releaser/pkg/cmd"
)

func main() {
	cmd.Execute(context.Background())
}
