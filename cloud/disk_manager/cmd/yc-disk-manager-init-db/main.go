package main

import (
	"context"
	"fmt"
	"io/ioutil"
	"log"

	"github.com/golang/protobuf/proto"
	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/auth"
	server_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/configs/server/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
)

////////////////////////////////////////////////////////////////////////////////

func parseConfig(
	configFileName string,
	config *server_config.ServerConfig,
) error {

	configBytes, err := ioutil.ReadFile(configFileName)
	if err != nil {
		return fmt.Errorf(
			"failed to read config file %v: %w",
			configFileName,
			err,
		)
	}

	err = proto.UnmarshalText(string(configBytes), config)
	if err != nil {
		return fmt.Errorf(
			"failed to parse config file %v as protobuf: %w",
			configFileName,
			err,
		)
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////

func run(
	config *server_config.ServerConfig,
	level logging.Level,
) error {

	ctx := context.Background()
	ctx = logging.SetLogger(ctx, logging.CreateStderrLogger(level))

	logging.Info(ctx, "Disk Manager init DB running")

	creds := auth.NewCredentials(ctx, config.GetAuthConfig())

	db, err := persistence.CreateYDBClient(
		ctx,
		config.GetPersistenceConfig(),
		persistence.WithCredentials(creds),
	)
	if err != nil {
		return fmt.Errorf("failed to connect to DB: %w", err)
	}
	defer db.Close(ctx)

	if config.GetDataplaneConfig() != nil {
		err = runDataplane(ctx, config, creds, db)
	} else {
		err = runControlplane(ctx, config, db)
	}
	if err != nil {
		return err
	}

	logging.Info(ctx, "Disk Manager init DB done")

	return nil
}

////////////////////////////////////////////////////////////////////////////////

func main() {
	var configFileName string
	var verbose bool
	config := &server_config.ServerConfig{}

	var rootCmd = &cobra.Command{
		Use:   "yc-disk-manager-init-db",
		Short: "DB management tool for Disk Manager service",
		PersistentPreRunE: func(cmd *cobra.Command, args []string) error {
			return parseConfig(configFileName, config)
		},
		RunE: func(cmd *cobra.Command, args []string) error {
			level := logging.InfoLevel
			if verbose {
				level = logging.DebugLevel
			}

			return run(config, level)
		},
	}
	rootCmd.Flags().StringVar(
		&configFileName,
		"config",
		"/etc/yc/disk-manager/server-config.txt",
		"Path to the config file",
	)
	rootCmd.Flags().BoolVarP(
		&verbose,
		"verbose",
		"v",
		false,
		"Enable verbose logging",
	)

	if err := rootCmd.Execute(); err != nil {
		log.Fatalf("Error: %v", err)
	}
}
