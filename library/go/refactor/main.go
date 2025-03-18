package main

import (
	"fmt"
	"os"

	"github.com/spf13/cobra"
)

var rootCmd = &cobra.Command{
	Use:   "refactor",
	Short: "refactor is an automation tool for global arcadia refactorings",
	Long:  `See library/go/refactor/README.md for usage examples`,
}

func init() {
	rootCmd.AddCommand(yolintCmd)
}

func main() {
	if err := rootCmd.Execute(); err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
}
