package main

import (
	"context"
	"fmt"
	"os"

	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/pkg/app"
	"a.yandex-team.ru/cloud/mdb/internal/flags"
)

var showHelp bool

func init() {
	pflag.BoolVarP(&showHelp, "help", "h", false, "Show help message")
	flags.RegisterConfigPathFlagGlobal()
	pflag.Usage = func() {
		fmt.Fprintf(os.Stderr, "Usage of %s:\n", os.Args[0])
		pflag.PrintDefaults()
	}
}

func main() {
	pflag.Parse()
	if showHelp {
		pflag.Usage()
		return
	}

	a, err := app.NewBackupBookkeeperAppFromConfig(context.Background())
	if err != nil {
		fmt.Fprintf(os.Stderr, "bookkeeper failed to start app: %s\n", err)
		os.Exit(1)
	}

	os.Exit(app.RunBilling(context.Background(), &a.BookkeeperApp))
}
