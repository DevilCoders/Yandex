package main

import (
	"fmt"
	"os"

	"a.yandex-team.ru/cloud/dataplatform/steadyprofiler/internal/cmd"
)

const (
	exitError = 1
)

func main() {
	command := cmd.New()

	if err := command.Execute(); err != nil {
		_, _ = fmt.Fprintln(os.Stderr, err)
		os.Exit(exitError)
	}
}
