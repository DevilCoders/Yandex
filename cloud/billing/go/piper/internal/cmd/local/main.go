package main

import (
	"log"
	"os"

	"github.com/spf13/cobra"
)

var (
	cmd = &cobra.Command{
		Use: "run",
		Run: run,
	}
	pushCmd = &cobra.Command{
		Use: "push",
		Run: pushData,
	}

	configFiles []string
)

func init() {
	cmd.PersistentFlags().StringArrayVarP(&configFiles, "config", "c", nil, "Path to the config file")
	_ = cmd.MarkPersistentFlagRequired("config")
}

func main() {
	defer globalStop()
	log.SetOutput(os.Stderr)

	if err := cmd.Execute(); err != nil {
		os.Exit(1)
	}
}
