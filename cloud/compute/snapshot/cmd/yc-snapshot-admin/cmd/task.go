package cmd

import (
	"encoding/json"
	"fmt"

	"github.com/spf13/cobra"
)

var (
	taskID string
)

func showAsJSON(data interface{}) error {
	jsonBytes, err := json.MarshalIndent(data, "", "  ")
	fmt.Printf("%s\n", jsonBytes)
	return err
}

// taskCmd represents the info command
var taskCmd = &cobra.Command{
	Use:   "task",
	Short: "Lists tasks, shows, cancels or deletes specified task",
}

var taskListCmd = &cobra.Command{
	Use:   "list",
	Short: "Lists current in manager tasks",
	Args:  cobra.ExactArgs(0),
	RunE: func(cmd *cobra.Command, args []string) error {
		ctx := createContext()
		client, err := snapshotClient(ctx)
		if err != nil {
			return err
		}
		if info, err := client.ListTasks(ctx); err == nil {
			return showAsJSON(info)
		} else {
			return err
		}

	},
}

var taskCancelCmd = &cobra.Command{
	Use:   "cancel --task-id <task-id>",
	Short: "Cancel task by id",
	Args:  cobra.ExactArgs(0),
	RunE: func(cmd *cobra.Command, args []string) error {
		ctx := createContext()
		client, err := snapshotClient(ctx)
		if err != nil {
			return err
		}
		if doAdminAction(taskID) {
			return client.CancelTask(ctx, taskID)
		} else {
			return nil
		}
	},
}

var taskDeleteCmd = &cobra.Command{
	Use:   "delete --task-id <task-id>",
	Short: "Delete task by id",
	Args:  cobra.ExactArgs(0),
	RunE: func(cmd *cobra.Command, args []string) error {
		ctx := createContext()
		client, err := snapshotClient(ctx)
		if err != nil {
			return err
		}
		if doAdminAction(taskID) {
			return client.CancelTask(ctx, taskID)
		} else {
			return nil
		}
	},
}

var taskShowCmd = &cobra.Command{
	Use:   "show --task-id <task-id>",
	Short: "Show task by id",
	Args:  cobra.ExactArgs(0),
	RunE: func(cmd *cobra.Command, args []string) error {
		ctx := createContext()
		client, err := snapshotClient(ctx)
		if err != nil {
			return err
		}
		if task, err := client.GetTask(ctx, taskID); err == nil {
			return showAsJSON(task)
		} else {
			return err
		}
	},
}

func init() {
	taskCmd.AddCommand(taskListCmd)

	taskCancelCmd.Flags().StringVar(&taskID, "task-id", "", "snapshot task id")
	_ = taskCancelCmd.MarkFlagRequired("task-id")
	taskCmd.AddCommand(taskCancelCmd)

	taskDeleteCmd.Flags().StringVar(&taskID, "task-id", "", "snapshot task id")
	_ = taskDeleteCmd.MarkFlagRequired("task-id")
	taskCmd.AddCommand(taskDeleteCmd)

	taskShowCmd.Flags().StringVar(&taskID, "task-id", "", "snapshot task id")
	_ = taskShowCmd.MarkFlagRequired("task-id")
	taskCmd.AddCommand(taskShowCmd)

	rootCmd.AddCommand(taskCmd)
}
