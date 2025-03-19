package main

import (
	"context"
	"fmt"
	"os"

	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/backup/scheduler/pkg/app"
	"a.yandex-team.ru/cloud/mdb/internal/flags"
)

var (
	opts            app.RunOpts
	showHelp        = false
	dryRun          = false
	rawClusterTypes []string
)

func init() {
	pflag.BoolVar(&opts.Plan, "schedule-create", opts.Plan, "Generate new backup records")
	pflag.BoolVar(&opts.Obsolete, "schedule-obsolete", opts.Obsolete, "Mark outdated backups to delete")
	pflag.BoolVar(&opts.Purge, "run-purge", opts.Purge, "Purge deleted backups from database")
	pflag.BoolVar(&dryRun, "dry-run", dryRun, "Do not perform actual job")
	pflag.StringSliceVar(&rawClusterTypes, "cluster-types", rawClusterTypes, "Handle provided cluster types")
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

	clusterTypes, err := metadb.ClusterTypesFromStrings(rawClusterTypes)
	if err != nil {
		fmt.Fprintf(os.Stderr, "failed to parse cluster-types: %+v\n", err)
		os.Exit(3)
	}

	os.Exit(app.Run(context.Background(), clusterTypes, opts, dryRun))
}
