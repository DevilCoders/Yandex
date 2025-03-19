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

type getSnapshot struct {
	clientConfig *client_config.ClientConfig
	serverConfig *server_config.ServerConfig

	snapshotID string
}

func (c *getSnapshot) run() error {
	ctx := createContextWithoutAuthHeader(c.clientConfig)

	resourceStorage, db, err := createResourceStorage(ctx, c.serverConfig)
	if err != nil {
		return err
	}
	defer db.Close(ctx)

	image, err := resourceStorage.GetSnapshotMeta(ctx, c.snapshotID)
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

func createGetSnapshotCmd(
	clientConfig *client_config.ClientConfig,
	serverConfig *server_config.ServerConfig,
) *cobra.Command {

	c := &getSnapshot{
		clientConfig: clientConfig,
		serverConfig: serverConfig,
	}

	cmd := &cobra.Command{
		Use: "get",
		RunE: func(cmd *cobra.Command, args []string) error {
			return c.run()
		},
	}

	cmd.Flags().StringVar(&c.snapshotID, "id", "", "ID of snapshot to get; required")
	if err := cmd.MarkFlagRequired("id"); err != nil {
		log.Fatalf("Error setting flag id as required: %v", err)
	}

	return cmd
}

////////////////////////////////////////////////////////////////////////////////

type listSnapshots struct {
	clientConfig *client_config.ClientConfig
	serverConfig *server_config.ServerConfig

	folderID string
}

func (c *listSnapshots) run() error {
	ctx := createContextWithoutAuthHeader(c.clientConfig)

	resourceStorage, db, err := createResourceStorage(ctx, c.serverConfig)
	if err != nil {
		return err
	}
	defer db.Close(ctx)

	ids, err := resourceStorage.ListSnapshots(ctx, c.folderID)
	if err != nil {
		return err
	}

	fmt.Println(strings.Join(ids, "\n"))

	return nil
}

func createListSnapshotsCmd(
	clientConfig *client_config.ClientConfig,
	serverConfig *server_config.ServerConfig,
) *cobra.Command {

	c := &listSnapshots{
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
		"ID of folder where snapshots are located; optional",
	)
	return cmd
}

////////////////////////////////////////////////////////////////////////////////

type createSnapshot struct {
	clientConfig  *client_config.ClientConfig
	zoneID        string
	srcDiskID     string
	dstSnapshotID string
	folderID      string
}

func (c *createSnapshot) run(clientConfig *client_config.ClientConfig) error {
	ctx, err := createContext(clientConfig)
	if err != nil {
		return fmt.Errorf("failed to create context: %w", err)
	}

	client, err := internal_client.NewClient(ctx, clientConfig)
	if err != nil {
		return fmt.Errorf("failed to create client: %w", err)
	}
	defer client.Close()

	req := &disk_manager.CreateSnapshotRequest{
		Src: &disk_manager.DiskId{
			ZoneId: c.zoneID,
			DiskId: c.srcDiskID,
		},
		SnapshotId: c.dstSnapshotID,
		FolderId:   c.folderID,
	}

	resp, err := client.CreateSnapshot(getRequestContext(ctx), req)
	if err != nil {
		return err
	}

	fmt.Printf("Operation: %v\n", resp.Id)

	return internal_client.WaitOperation(ctx, client, resp.Id)
}

func createCreateSnapshotCmd(clientConfig *client_config.ClientConfig) *cobra.Command {
	c := &createSnapshot{
		clientConfig: clientConfig,
	}

	cmd := &cobra.Command{
		Use: "create",
		RunE: func(cmd *cobra.Command, args []string) error {
			return c.run(clientConfig)
		},
	}

	cmd.Flags().StringVar(&c.zoneID, "zone-id", "", "zone ID in which disk is located; required")
	if err := cmd.MarkFlagRequired("zone-id"); err != nil {
		log.Fatalf("Error setting flag zone-id as required: %v", err)
	}

	cmd.Flags().StringVar(&c.srcDiskID, "src-disk-id", "", "ID of disk to create snapshot from; required")
	if err := cmd.MarkFlagRequired("src-disk-id"); err != nil {
		log.Fatalf("Error setting flag src-disk-id as required: %v", err)
	}

	cmd.Flags().StringVar(&c.dstSnapshotID, "id", "", "ID of snapshot to create; required")
	if err := cmd.MarkFlagRequired("id"); err != nil {
		log.Fatalf("Error setting flag id as required: %v", err)
	}

	cmd.Flags().StringVar(&c.folderID, "folder-id", "", "folder ID of the snapshot owner; required")
	if err := cmd.MarkFlagRequired("folder-id"); err != nil {
		log.Fatalf("Error setting flag folder-id as required: %v", err)
	}

	return cmd
}

////////////////////////////////////////////////////////////////////////////////

type deleteSnapshot struct {
	clientConfig *client_config.ClientConfig
	snapshotID   string
}

func (c *deleteSnapshot) run(clientConfig *client_config.ClientConfig) error {
	ctx, err := createContext(clientConfig)
	if err != nil {
		return fmt.Errorf("failed to create context: %w", err)
	}

	err = requestDeletionConfirmation("snapshot", c.snapshotID)
	if err != nil {
		return err
	}

	client, err := internal_client.NewClient(ctx, clientConfig)
	if err != nil {
		return fmt.Errorf("failed to create client: %w", err)
	}
	defer client.Close()

	req := &disk_manager.DeleteSnapshotRequest{
		SnapshotId: c.snapshotID,
	}

	resp, err := client.DeleteSnapshot(getRequestContext(ctx), req)
	if err != nil {
		return err
	}

	fmt.Printf("Operation: %v\n", resp.Id)

	return internal_client.WaitOperation(ctx, client, resp.Id)
}

func createDeleteSnapshotCmd(clientConfig *client_config.ClientConfig) *cobra.Command {
	c := &deleteSnapshot{
		clientConfig: clientConfig,
	}

	cmd := &cobra.Command{
		Use: "delete",
		RunE: func(cmd *cobra.Command, args []string) error {
			return c.run(clientConfig)
		},
	}

	cmd.Flags().StringVar(&c.snapshotID, "id", "", "ID of snapshot to delete; required")
	if err := cmd.MarkFlagRequired("id"); err != nil {
		log.Fatalf("Error setting flag id as required: %v", err)
	}

	return cmd
}

////////////////////////////////////////////////////////////////////////////////

type restoreDiskFromSnapshot struct {
	clientConfig *client_config.ClientConfig
	snapshotID   string
	diskID       string
	zoneID       string
}

func (c *restoreDiskFromSnapshot) run(clientConfig *client_config.ClientConfig) error {
	ctx, err := createContext(clientConfig)
	if err != nil {
		return fmt.Errorf("failed to create context: %w", err)
	}

	client, err := internal_client.NewClient(ctx, clientConfig)
	if err != nil {
		return fmt.Errorf("failed to create client: %w", err)
	}
	defer client.Close()

	req := &disk_manager.RestoreDiskFromSnapshotRequest{
		SnapshotId: c.snapshotID,
		DiskId: &disk_manager.DiskId{
			ZoneId: c.zoneID,
			DiskId: c.diskID,
		},
	}

	resp, err := client.RestoreDiskFromSnapshot(getRequestContext(ctx), req)
	if err != nil {
		return err
	}

	fmt.Printf("Operation: %v\n", resp.Id)

	return internal_client.WaitOperation(ctx, client, resp.Id)
}

func createRestoreDiskFromSnapshotCmd(clientConfig *client_config.ClientConfig) *cobra.Command {
	c := &restoreDiskFromSnapshot{
		clientConfig: clientConfig,
	}

	cmd := &cobra.Command{
		Use: "restore_disk",
		RunE: func(cmd *cobra.Command, args []string) error {
			return c.run(clientConfig)
		},
	}

	cmd.Flags().StringVar(&c.snapshotID, "snapshot-id", "", "ID of snapshot to restore disk from; required")
	if err := cmd.MarkFlagRequired("snapshot-id"); err != nil {
		log.Fatalf("Error setting flag snapshot-id as required: %v", err)
	}

	cmd.Flags().StringVar(&c.zoneID, "zone-id", "", "zone ID in which disk is located; required")
	if err := cmd.MarkFlagRequired("zone-id"); err != nil {
		log.Fatalf("Error setting flag zone-id as required: %v", err)
	}

	cmd.Flags().StringVar(&c.diskID, "disk-id", "", "ID of disk to restore; required")
	if err := cmd.MarkFlagRequired("disk-id"); err != nil {
		log.Fatalf("Error setting flag disk-id as required: %v", err)
	}

	return cmd
}

////////////////////////////////////////////////////////////////////////////////

func createSnapshotsCmd(
	clientConfig *client_config.ClientConfig,
	serverConfig *server_config.ServerConfig,
) *cobra.Command {

	cmd := &cobra.Command{
		Use: "snapshots",
	}

	cmd.AddCommand(
		createGetSnapshotCmd(clientConfig, serverConfig),
		createListSnapshotsCmd(clientConfig, serverConfig),
		createCreateSnapshotCmd(clientConfig),
		createDeleteSnapshotCmd(clientConfig),
		createRestoreDiskFromSnapshotCmd(clientConfig),
	)

	return cmd
}
