package main

import (
	"fmt"
	"os"

	"github.com/spf13/cobra"

	mdb_porto "a.yandex-team.ru/cloud/mdb/mdb-selfdns-client-plugins/internal/mdb-porto"
)

var rootCmd = &cobra.Command{
	Use:   "mdb-porto",
	Short: "mdb-porto is a selfdns-client plugin for porto containers environment in MDB",
	Run: func(cmd *cobra.Command, args []string) {
		mdb_porto.Run()
	},
}

func main() {
	if err := rootCmd.Execute(); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}
