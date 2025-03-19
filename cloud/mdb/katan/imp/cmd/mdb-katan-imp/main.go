package main

import (
	"fmt"
	"os"

	"a.yandex-team.ru/cloud/mdb/katan/imp/pkg/app"
)

func main() {
	if len(os.Args) != 2 {
		fmt.Fprintf(os.Stderr, "Usage %s <config file>\n", os.Args[0])
		os.Exit(2)
	}
	os.Exit(app.Run(os.Args[1]))
}
