package main

import (
	"encoding/json"
	"fmt"
	"log"
	"strings"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	internal_client "a.yandex-team.ru/cloud/disk_manager/internal/pkg/client"
	client_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/configs/client/config"
	server_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/configs/server/config"
)

////////////////////////////////////////////////////////////////////////////////

type getPlacementGroup struct {
	clientConfig     *client_config.ClientConfig
	serverConfig     *server_config.ServerConfig
	placementGroupID string
}

func (c *getPlacementGroup) run() error {
	ctx := createContextWithoutAuthHeader(c.clientConfig)

	resourceStorage, db, err := createResourceStorage(ctx, c.serverConfig)
	if err != nil {
		return err
	}
	defer db.Close(ctx)

	image, err := resourceStorage.GetPlacementGroupMeta(ctx, c.placementGroupID)
	if err != nil {
		return err
	}

	j, err := json.Marshal(image)
	if err != nil {
		return err
	}

	fmt.Println(string(j))

	return nil
}

func createGetPlacementGroupCmd(
	clientConfig *client_config.ClientConfig,
	serverConfig *server_config.ServerConfig,
) *cobra.Command {

	c := &getPlacementGroup{
		clientConfig: clientConfig,
		serverConfig: serverConfig,
	}

	cmd := &cobra.Command{
		Use: "get",
		RunE: func(cmd *cobra.Command, args []string) error {
			return c.run()
		},
	}

	cmd.Flags().StringVar(
		&c.placementGroupID,
		"id",
		"",
		"placementGroup ID; required")
	if err := cmd.MarkFlagRequired("id"); err != nil {
		log.Fatalf("Error setting flag id as required: %v", err)
	}

	return cmd
}

////////////////////////////////////////////////////////////////////////////////

type listPlacementGroups struct {
	clientConfig *client_config.ClientConfig
	serverConfig *server_config.ServerConfig

	folderID string
}

func (c *listPlacementGroups) run() error {
	ctx := createContextWithoutAuthHeader(c.clientConfig)

	resourceStorage, db, err := createResourceStorage(ctx, c.serverConfig)
	if err != nil {
		return err
	}
	defer db.Close(ctx)

	ids, err := resourceStorage.ListPlacementGroups(ctx, c.folderID)
	if err != nil {
		return err
	}

	fmt.Println(strings.Join(ids, "\n"))

	return nil
}

func createListPlacementGroupsCmd(
	clientConfig *client_config.ClientConfig,
	serverConfig *server_config.ServerConfig,
) *cobra.Command {

	c := &listPlacementGroups{
		clientConfig: clientConfig,
		serverConfig: serverConfig,
	}

	cmd := &cobra.Command{
		Use: "list",
		RunE: func(cmd *cobra.Command, args []string) error {
			return c.run()
		},
	}

	cmd.Flags().StringVar(
		&c.folderID,
		"folder-id",
		"",
		"ID of folder where placement groups are located; optional",
	)
	return cmd
}

////////////////////////////////////////////////////////////////////////////////

type createPlacementGroup struct {
	clientConfig     *client_config.ClientConfig
	zoneID           string
	placementGroupID string
}

func (c *createPlacementGroup) run(
	clientConfig *client_config.ClientConfig,
) error {

	ctx, err := createContext(clientConfig)
	if err != nil {
		return fmt.Errorf("failed to create context: %w", err)
	}

	client, err := internal_client.NewClient(ctx, clientConfig)
	if err != nil {
		return fmt.Errorf("failed to create client: %w", err)
	}
	defer client.Close()

	req := &disk_manager.CreatePlacementGroupRequest{
		GroupId: &disk_manager.GroupId{
			ZoneId:  c.zoneID,
			GroupId: c.placementGroupID,
		},
		PlacementStrategy: disk_manager.PlacementStrategy_PLACEMENT_STRATEGY_SPREAD,
	}

	resp, err := client.CreatePlacementGroup(getRequestContext(ctx), req)
	if err != nil {
		return err
	}

	fmt.Printf("Operation: %v\n", resp.Id)

	return internal_client.WaitOperation(ctx, client, resp.Id)
}

func createCreatePlacementGroupCmd(
	clientConfig *client_config.ClientConfig,
) *cobra.Command {

	c := &createPlacementGroup{
		clientConfig: clientConfig,
	}

	cmd := &cobra.Command{
		Use: "create",
		RunE: func(cmd *cobra.Command, args []string) error {
			return c.run(clientConfig)
		},
	}

	cmd.Flags().StringVar(
		&c.zoneID,
		"zone-id",
		"",
		"zone ID in which to create placementGroup; required")
	if err := cmd.MarkFlagRequired("zone-id"); err != nil {
		log.Fatalf("Error setting flag zone-id as required: %v", err)
	}

	cmd.Flags().StringVar(
		&c.placementGroupID,
		"id",
		"",
		"placementGroup ID; required")
	if err := cmd.MarkFlagRequired("id"); err != nil {
		log.Fatalf("Error setting flag id as required: %v", err)
	}

	return cmd
}

////////////////////////////////////////////////////////////////////////////////

type deletePlacementGroup struct {
	clientConfig     *client_config.ClientConfig
	zoneID           string
	placementGroupID string
}

func (c *deletePlacementGroup) run(
	clientConfig *client_config.ClientConfig,
) error {

	ctx, err := createContext(clientConfig)
	if err != nil {
		return fmt.Errorf("failed to create context: %w", err)
	}

	err = requestDeletionConfirmation("placementGroup", c.placementGroupID)
	if err != nil {
		return err
	}

	client, err := internal_client.NewClient(ctx, clientConfig)
	if err != nil {
		return fmt.Errorf("failed to create client: %w", err)
	}
	defer client.Close()

	req := &disk_manager.DeletePlacementGroupRequest{
		GroupId: &disk_manager.GroupId{
			ZoneId:  c.zoneID,
			GroupId: c.placementGroupID,
		},
	}

	resp, err := client.DeletePlacementGroup(getRequestContext(ctx), req)
	if err != nil {
		return err
	}

	fmt.Printf("Operation: %v\n", resp.Id)

	return internal_client.WaitOperation(ctx, client, resp.Id)
}

func createDeletePlacementGroupCmd(
	clientConfig *client_config.ClientConfig,
) *cobra.Command {

	c := &deletePlacementGroup{
		clientConfig: clientConfig,
	}

	cmd := &cobra.Command{
		Use: "delete",
		RunE: func(cmd *cobra.Command, args []string) error {
			return c.run(clientConfig)
		},
	}

	cmd.Flags().StringVar(
		&c.zoneID,
		"zone-id",
		"",
		"zone ID where placementGroup is located; required")
	if err := cmd.MarkFlagRequired("zone-id"); err != nil {
		log.Fatalf("Error setting flag zone-id as required: %v", err)
	}

	cmd.Flags().StringVar(
		&c.placementGroupID,
		"id",
		"",
		"placementGroup ID; required")
	if err := cmd.MarkFlagRequired("id"); err != nil {
		log.Fatalf("Error setting flag id as required: %v", err)
	}

	return cmd
}

////////////////////////////////////////////////////////////////////////////////

func createPlacementGroupCmd(
	clientConfig *client_config.ClientConfig,
	serverConfig *server_config.ServerConfig,
) *cobra.Command {

	cmd := &cobra.Command{
		Use: "placementGroup",
	}

	cmd.AddCommand(
		createGetPlacementGroupCmd(clientConfig, serverConfig),
		createListPlacementGroupsCmd(clientConfig, serverConfig),
		createCreatePlacementGroupCmd(clientConfig),
		createDeletePlacementGroupCmd(clientConfig),
	)

	return cmd
}
