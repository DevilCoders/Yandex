package main

import (
	"context"
	"fmt"
	"log"
	"time"

	"github.com/spf13/cobra"

	client_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/configs/client/config"
	server_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/configs/server/config"
	tasks_storage "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/storage"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/util"
)

func toJSONWithDependencies(
	ctx context.Context,
	task *tasks_storage.TaskState,
	taskStorage tasks_storage.Storage,
	maxDepth uint64,
) (*util.TaskStateJSON, error) {

	jsonTask := util.TaskStateToJSON(task)

	if maxDepth != 0 {
		for i, depTask := range jsonTask.Dependencies {
			t, err := taskStorage.GetTask(ctx, depTask.ID)
			if err != nil {
				return nil, fmt.Errorf("failed to get task: %w", err)
			}

			stateJSON, err := toJSONWithDependencies(ctx, &t, taskStorage, maxDepth-1)
			if err != nil {
				return nil, fmt.Errorf("failed to expand dependencies: %w", err)
			}

			jsonTask.Dependencies[i] = stateJSON
		}
	}

	return jsonTask, nil
}

////////////////////////////////////////////////////////////////////////////////

type getTask struct {
	clientConfig *client_config.ClientConfig
	serverConfig *server_config.ServerConfig
	taskID       string
	maxDepth     uint64
}

func (c *getTask) run() error {
	ctx := createContextWithoutAuthHeader(c.clientConfig)

	taskStorage, db, err := createTaskStorage(ctx, c.serverConfig)
	if err != nil {
		return err
	}
	defer db.Close(ctx)

	task, err := taskStorage.GetTask(ctx, c.taskID)
	if err != nil {
		return fmt.Errorf("failed to get task: %w", err)
	}

	jsonTask, err := toJSONWithDependencies(ctx, &task, taskStorage, c.maxDepth)
	if err != nil {
		return err
	}

	json, err := jsonTask.Marshal()
	if err != nil {
		return fmt.Errorf("failed to marshal task to json: %w", err)
	}

	fmt.Println(string(json))

	return nil
}

func createGetTaskCmd(
	clientConfig *client_config.ClientConfig,
	serverConfig *server_config.ServerConfig,
) *cobra.Command {

	c := &getTask{
		clientConfig: clientConfig,
		serverConfig: serverConfig,
	}

	cmd := &cobra.Command{
		Use: "get",
		RunE: func(cmd *cobra.Command, args []string) error {
			return c.run()
		},
	}

	cmd.Flags().StringVar(&c.taskID, "id", "", "ID of task to get")
	if err := cmd.MarkFlagRequired("id"); err != nil {
		log.Fatalf("Error setting flag id as required: %v", err)
	}

	cmd.Flags().Uint64Var(&c.maxDepth, "max-depth", 10, "Max depth of expanding dependencies")

	return cmd
}

////////////////////////////////////////////////////////////////////////////////

type cancelTask struct {
	clientConfig *client_config.ClientConfig
	serverConfig *server_config.ServerConfig
	taskID       string
}

func (c *cancelTask) run() error {
	ctx := createContextWithoutAuthHeader(c.clientConfig)

	taskStorage, db, err := createTaskStorage(ctx, c.serverConfig)
	if err != nil {
		return err
	}
	defer db.Close(ctx)

	ok, err := taskStorage.MarkForCancellation(ctx, c.taskID, time.Now())
	if err != nil {
		return fmt.Errorf("failed to mark for cancellation task: %w", err)
	}

	if ok {
		fmt.Printf("Task %s has marked for cancellation\n", c.taskID)
	} else {
		fmt.Printf("Task %s hasn't marked for cancellation. Maybe it has already finished\n", c.taskID)
	}

	return nil
}

func createCancelTaskCmd(
	clientConfig *client_config.ClientConfig,
	serverConfig *server_config.ServerConfig,
) *cobra.Command {

	c := &cancelTask{
		clientConfig: clientConfig,
		serverConfig: serverConfig,
	}

	cmd := &cobra.Command{
		Use: "cancel",
		RunE: func(cmd *cobra.Command, args []string) error {
			return c.run()
		},
	}

	cmd.Flags().StringVar(&c.taskID, "id", "", "ID of task to get")
	if err := cmd.MarkFlagRequired("id"); err != nil {
		log.Fatalf("Error setting flag id as required: %v", err)
	}

	return cmd
}

////////////////////////////////////////////////////////////////////////////////

type lister func(context.Context, tasks_storage.Storage, uint64) ([]tasks_storage.TaskInfo, error)

type listTask struct {
	clientConfig *client_config.ClientConfig
	serverConfig *server_config.ServerConfig
	taskLister   lister
	limit        uint64
}

func (c *listTask) run() error {
	ctx := createContextWithoutAuthHeader(c.clientConfig)

	taskStorage, db, err := createTaskStorage(ctx, c.serverConfig)
	if err != nil {
		return err
	}
	defer db.Close(ctx)

	taskInfos, err := c.taskLister(ctx, taskStorage, c.limit)
	if err != nil {
		return fmt.Errorf("failed to list ready to run task: %w", err)
	}

	for _, taskInfo := range taskInfos {
		task, err := taskStorage.GetTask(ctx, taskInfo.ID)
		if err != nil {
			return fmt.Errorf("failed to get task: %w", err)
		}

		json, err := util.TaskStateToJSON(&task).Marshal()
		if err != nil {
			return fmt.Errorf("failed to marshal task to json: %w", err)
		}

		fmt.Println(string(json))
	}

	return nil
}

func createListTaskCmd(
	clientConfig *client_config.ClientConfig,
	serverConfig *server_config.ServerConfig,
	command string,
	taskLister lister,
) *cobra.Command {

	c := &listTask{
		clientConfig: clientConfig,
		serverConfig: serverConfig,
		taskLister:   taskLister,
	}

	cmd := &cobra.Command{
		Use: command,
		RunE: func(cmd *cobra.Command, args []string) error {
			return c.run()
		},
	}

	cmd.Flags().Uint64Var(&c.limit, "limit", 10, "limit for listing tasks")

	return cmd
}

func createListReadyToRunCmd(
	clientConfig *client_config.ClientConfig,
	serverConfig *server_config.ServerConfig,
) *cobra.Command {

	return createListTaskCmd(
		clientConfig,
		serverConfig,
		"ready_to_run",
		func(ctx context.Context, storage tasks_storage.Storage, limit uint64) ([]tasks_storage.TaskInfo, error) {
			return storage.ListTasksReadyToRun(ctx, limit)
		})
}

func createListReadyToCancelCmd(
	clientConfig *client_config.ClientConfig,
	serverConfig *server_config.ServerConfig,
) *cobra.Command {

	return createListTaskCmd(
		clientConfig,
		serverConfig,
		"ready_to_cancel",
		func(ctx context.Context, storage tasks_storage.Storage, limit uint64) ([]tasks_storage.TaskInfo, error) {
			return storage.ListTasksReadyToRun(ctx, limit)
		})
}

func createListRunningCmd(
	clientConfig *client_config.ClientConfig,
	serverConfig *server_config.ServerConfig,
) *cobra.Command {

	return createListTaskCmd(
		clientConfig,
		serverConfig,
		"running",
		func(ctx context.Context, storage tasks_storage.Storage, limit uint64) ([]tasks_storage.TaskInfo, error) {
			return storage.ListTasksRunning(ctx, limit)
		})
}

func createListCancellingCmd(
	clientConfig *client_config.ClientConfig,
	serverConfig *server_config.ServerConfig,
) *cobra.Command {

	return createListTaskCmd(
		clientConfig,
		serverConfig,
		"cancelling",
		func(ctx context.Context, storage tasks_storage.Storage, limit uint64) ([]tasks_storage.TaskInfo, error) {
			return storage.ListTasksCancelling(ctx, limit)
		})
}

////////////////////////////////////////////////////////////////////////////////

func createListCmd(
	clientConfig *client_config.ClientConfig,
	serverConfig *server_config.ServerConfig,
) *cobra.Command {

	cmd := &cobra.Command{
		Use: "list",
	}

	cmd.AddCommand(
		createListReadyToRunCmd(clientConfig, serverConfig),
		createListReadyToCancelCmd(clientConfig, serverConfig),
		createListRunningCmd(clientConfig, serverConfig),
		createListCancellingCmd(clientConfig, serverConfig),
	)

	return cmd
}

////////////////////////////////////////////////////////////////////////////////

func createTasksCmd(
	clientConfig *client_config.ClientConfig,
	serverConfig *server_config.ServerConfig,
) *cobra.Command {

	cmd := &cobra.Command{
		Use: "tasks",
	}

	cmd.AddCommand(
		createGetTaskCmd(clientConfig, serverConfig),
		createCancelTaskCmd(clientConfig, serverConfig),
		createListCmd(clientConfig, serverConfig),
	)

	return cmd
}
