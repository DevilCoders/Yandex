package main

import (
	"fmt"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/marketplace/license_server/dev/lsc/migration"
)

var (
	appConfigsPath []string
)

var rootCommand = &cobra.Command{
	Use:   "lcl",
	Short: "Run License Check CLI",
}

var migrateCommand = &cobra.Command{
	Use:   "migrate",
	Short: "Migration command",
}

var migrateInitCommand = &cobra.Command{
	Use:   "init",
	Short: "Init command",
}

var migrateInitUpCommand = &cobra.Command{
	Use:   "up",
	Short: "up command",

	Run: runMigrateInitUpCommand,
}

var migrateInitDownCommand = &cobra.Command{
	Use:   "down",
	Short: "down command",

	Run: runMigrateInitDownCommand,
}

var migrateUpCommand = &cobra.Command{
	Use:   "up",
	Short: "up migration",

	Run: runMigrateUpCommand,
}

var migrateDownCommand = &cobra.Command{
	Use:   "down",
	Short: "down migration",

	Run: runMigrateDownCommand,
}

func init() {
	migrateCommand.PersistentFlags().StringArrayVarP(
		&appConfigsPath, "config", "c", nil, "service config files paths",
	)

	migrateInitCommand.AddCommand(
		migrateInitUpCommand,
		migrateInitDownCommand,
	)
	migrateCommand.AddCommand(
		migrateInitCommand,
		migrateUpCommand,
		migrateDownCommand,
	)
	rootCommand.AddCommand(
		migrateCommand,
	)
}

func runMigrateInitUpCommand(cmd *cobra.Command, args []string) {
	err := migration.MigrateInitUp(appConfigsPath)
	fmt.Println(err)
}

func runMigrateInitDownCommand(cmd *cobra.Command, args []string) {
	err := migration.MigrateInitDown(appConfigsPath)
	fmt.Println(err)
}

func runMigrateUpCommand(cmd *cobra.Command, args []string) {
	err := migration.MigrateUp(appConfigsPath)
	fmt.Println(err)
}

func runMigrateDownCommand(cmd *cobra.Command, args []string) {
	err := migration.MigrateDown(appConfigsPath)
	fmt.Println(err)
}
