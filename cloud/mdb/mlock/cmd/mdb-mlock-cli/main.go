package main

import (
	"fmt"
	"os"

	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/cloud/mdb/internal/tilde"
	"a.yandex-team.ru/cloud/mdb/mlock/internal/client"
)

const (
	knownActions = "create, release, status, list"
)

var (
	configPath, lockID, holder, reason string
	limit                              int64
)

func init() {
	defaultConfigPath, err := tilde.Expand("~/.mlock-cli.yaml")
	if err != nil {
		defaultConfigPath = ""
	}
	flags := pflag.NewFlagSet("Mlock client", pflag.ExitOnError)
	flags.StringVarP(&configPath, "config", "c", defaultConfigPath, "Config path")
	flags.StringVarP(&lockID, "lock-id", "i", "", "Lock id")
	flags.StringVarP(&holder, "holder", "h", "", "Lock holder")
	flags.StringVarP(&reason, "reason", "r", "", "Lock reason")
	flags.Int64VarP(&limit, "limit", "l", int64(0), "Per request limit for listing")

	pflag.CommandLine.AddFlagSet(flags)

	pflag.Usage = func() {
		fmt.Fprintf(os.Stderr, "Usage: %s <action (%s)> [flags] [object ...]\n", os.Args[0], knownActions)
		pflag.PrintDefaults()
	}
}

func main() {
	pflag.Parse()
	args := pflag.Args()

	if len(args) < 1 {
		pflag.Usage()
		os.Exit(1)
	}

	conf := client.DefaultConfig()

	err := config.LoadFromAbsolutePath(configPath, &conf)

	if err != nil {
		panic(err)
	}

	client, err := client.NewFromConfig(conf)

	if err != nil {
		panic(err)
	}

	switch args[0] {
	case "create":
		if lockID == "" {
			fmt.Fprintln(os.Stderr, "lock-id is required for create")
			os.Exit(1)
		}
		if holder == "" {
			fmt.Fprintln(os.Stderr, "holder is required for create")
			os.Exit(1)
		}
		if reason == "" {
			fmt.Fprintln(os.Stderr, "reason is required for create")
			os.Exit(1)
		}
		if len(args) < 2 {
			fmt.Fprintln(os.Stderr, "empty object list")
			os.Exit(1)
		}
		os.Exit(client.Create(lockID, holder, reason, args[1:]))
	case "release":
		if lockID == "" {
			fmt.Fprintln(os.Stderr, "lock-id is required for release")
			os.Exit(1)
		}
		os.Exit(client.Release(lockID))
	case "status":
		if lockID == "" {
			fmt.Fprintln(os.Stderr, "lock-id is required for status")
			os.Exit(1)
		}
		os.Exit(client.Status(lockID))
	case "list":
		if limit < 0 {
			fmt.Fprintf(os.Stderr, "non-positive value for limit: %d\n", limit)
			os.Exit(1)
		}
		os.Exit(client.List(holder, limit))
	default:
		fmt.Fprintf(os.Stderr, "Unknown action: %s. Known actions: %s\n", args[0], knownActions)
		os.Exit(1)
	}
}
