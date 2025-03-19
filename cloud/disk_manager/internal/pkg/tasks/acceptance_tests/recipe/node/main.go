package main

import (
	"context"
	"fmt"
	"io/ioutil"
	"log"

	"github.com/golang/protobuf/proto"
	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring/metrics"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	node_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/acceptance_tests/recipe/node/config"
	recipe_tasks "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/acceptance_tests/recipe/tasks"
	tasks_storage "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/storage"
)

////////////////////////////////////////////////////////////////////////////////

func parseConfig(
	configFileName string,
	config *node_config.Config,
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

func run(config *node_config.Config) error {
	logger := logging.CreateLogger(config.GetLoggingConfig())
	ctx := logging.SetLogger(context.Background(), logger)

	db, err := persistence.CreateYDBClient(ctx, config.GetPersistenceConfig())
	if err != nil {
		return err
	}
	defer db.Close(ctx)

	storage, err := tasks_storage.CreateStorage(
		config.GetTasksConfig(),
		metrics.CreateEmptyRegistry(),
		db,
	)
	if err != nil {
		return err
	}

	registry := tasks.CreateRegistry()

	scheduler, err := tasks.CreateScheduler(
		ctx,
		registry,
		storage,
		config.GetTasksConfig(),
	)
	if err != nil {
		return err
	}

	err = registry.Register(ctx, "ChainTask", func() tasks.Task {
		return recipe_tasks.NewChainTask(scheduler)
	})
	if err != nil {
		return err
	}

	err = tasks.StartRunners(
		ctx,
		storage,
		registry,
		metrics.CreateEmptyRegistry(),
		config.GetTasksConfig(),
		config.GetHostname(),
	)
	if err != nil {
		return err
	}

	select {}
}

////////////////////////////////////////////////////////////////////////////////

func main() {
	var configFileName string
	config := &node_config.Config{}

	rootCmd := &cobra.Command{
		PersistentPreRunE: func(cmd *cobra.Command, args []string) error {
			return parseConfig(configFileName, config)
		},
		RunE: func(cmd *cobra.Command, args []string) error {
			return run(config)
		},
	}

	rootCmd.Flags().StringVar(
		&configFileName,
		"config",
		"",
		"Path to the config file",
	)

	if err := rootCmd.Execute(); err != nil {
		log.Fatalf("Failed to run: %v", err)
	}
}
