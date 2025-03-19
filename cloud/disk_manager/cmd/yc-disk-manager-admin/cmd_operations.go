package main

import (
	"fmt"
	"log"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/client"
	client_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/configs/client/config"
)

////////////////////////////////////////////////////////////////////////////////

type getOperation struct {
	config      *client_config.ClientConfig
	operationID string
}

func (c *getOperation) run(config *client_config.ClientConfig) error {
	ctx, err := createContext(config)
	if err != nil {
		return fmt.Errorf("failed to create context: %w", err)
	}

	client, err := client.NewClient(ctx, config)
	if err != nil {
		return fmt.Errorf("failed to create client: %w", err)
	}
	defer client.Close()

	resp, err := client.GetOperation(
		getRequestContext(ctx),
		&disk_manager.GetOperationRequest{
			OperationId: c.operationID,
		},
	)
	if err != nil {
		return err
	}

	fmt.Printf("Operation: %v\n", resp)
	return nil
}

func createGetOperationCmd(config *client_config.ClientConfig) *cobra.Command {
	c := &getOperation{
		config: config,
	}

	cmd := &cobra.Command{
		Use: "get",
		RunE: func(cmd *cobra.Command, args []string) error {
			return c.run(config)
		},
	}

	cmd.Flags().StringVar(&c.operationID, "id", "", "ID of operation to get")
	if err := cmd.MarkFlagRequired("id"); err != nil {
		log.Fatalf("Error setting flag id as required: %v", err)
	}

	return cmd
}

////////////////////////////////////////////////////////////////////////////////

type cancelOperation struct {
	config      *client_config.ClientConfig
	operationID string
}

func (c *cancelOperation) run(config *client_config.ClientConfig) error {
	ctx, err := createContext(config)
	if err != nil {
		return fmt.Errorf("failed to create context: %w", err)
	}

	client, err := client.NewClient(ctx, config)
	if err != nil {
		return fmt.Errorf("failed to create client: %w", err)
	}
	defer client.Close()

	resp, err := client.CancelOperation(
		getRequestContext(ctx),
		&disk_manager.CancelOperationRequest{
			OperationId: c.operationID,
		},
	)
	if err != nil {
		return err
	}

	fmt.Printf("Operation: %v\n", resp)
	return nil
}

func createCancelOperationCmd(config *client_config.ClientConfig) *cobra.Command {
	c := &cancelOperation{
		config: config,
	}

	cmd := &cobra.Command{
		Use: "cancel",
		RunE: func(cmd *cobra.Command, args []string) error {
			return c.run(config)
		},
	}

	cmd.Flags().StringVar(&c.operationID, "id", "", "ID of operation to cancel")
	if err := cmd.MarkFlagRequired("id"); err != nil {
		log.Fatalf("Error setting flag id as required: %v", err)
	}

	return cmd
}

////////////////////////////////////////////////////////////////////////////////

func createOperationsCmd(config *client_config.ClientConfig) *cobra.Command {
	cmd := &cobra.Command{
		Use: "operations",
	}

	cmd.AddCommand(
		createGetOperationCmd(config),
		createCancelOperationCmd(config),
	)

	return cmd
}
