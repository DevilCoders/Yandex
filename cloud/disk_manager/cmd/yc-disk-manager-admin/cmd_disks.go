package main

import (
	"encoding/json"
	"fmt"
	"log"
	"strings"

	"github.com/golang/protobuf/ptypes/empty"
	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	internal_client "a.yandex-team.ru/cloud/disk_manager/internal/pkg/client"
	client_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/configs/client/config"
	server_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/configs/server/config"
)

////////////////////////////////////////////////////////////////////////////////

type diskKind disk_manager.DiskKind

var stringToDiskKind = map[string]diskKind{
	"ssd": diskKind(disk_manager.DiskKind_DISK_KIND_SSD),
	"hdd": diskKind(disk_manager.DiskKind_DISK_KIND_HDD),
	"ssd-nonreplicated": diskKind(
		disk_manager.DiskKind_DISK_KIND_SSD_NONREPLICATED),
	"nonreplicated": diskKind(
		disk_manager.DiskKind_DISK_KIND_SSD_NONREPLICATED),
	"mirror2":     diskKind(disk_manager.DiskKind_DISK_KIND_SSD_MIRROR2),
	"ssd-mirror2": diskKind(disk_manager.DiskKind_DISK_KIND_SSD_MIRROR2),
	"mirror3":     diskKind(disk_manager.DiskKind_DISK_KIND_SSD_MIRROR3),
	"ssd-mirror3": diskKind(disk_manager.DiskKind_DISK_KIND_SSD_MIRROR3),
	"local":       diskKind(disk_manager.DiskKind_DISK_KIND_SSD_LOCAL),
	"ssd-local":   diskKind(disk_manager.DiskKind_DISK_KIND_SSD_LOCAL),
}

func (k *diskKind) String() string {
	for name, val := range stringToDiskKind {
		if val == *k {
			return name
		}
	}

	log.Fatalf("Unknown disk kind: %v", *k)
	return ""
}

func (k *diskKind) Set(val string) error {
	var ok bool
	*k, ok = stringToDiskKind[val]
	if !ok {
		return fmt.Errorf("unknown kind %v", val)
	}

	return nil
}

func (k *diskKind) Type() string {
	return "DiskKind"
}

////////////////////////////////////////////////////////////////////////////////

type agentIds []string

func (ids *agentIds) String() string {
	return fmt.Sprint(*ids)
}

func (ids *agentIds) Set(val string) error {
	*ids = append(*ids, val)
	return nil
}

func (ids *agentIds) Type() string {
	return "AgentIDs"
}

////////////////////////////////////////////////////////////////////////////////

type getDisk struct {
	clientConfig *client_config.ClientConfig
	serverConfig *server_config.ServerConfig

	diskID string
}

func (c *getDisk) run() error {
	ctx := createContextWithoutAuthHeader(c.clientConfig)

	resourceStorage, db, err := createResourceStorage(ctx, c.serverConfig)
	if err != nil {
		return err
	}
	defer db.Close(ctx)

	image, err := resourceStorage.GetDiskMeta(ctx, c.diskID)
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

func createGetDiskCmd(
	clientConfig *client_config.ClientConfig,
	serverConfig *server_config.ServerConfig,
) *cobra.Command {

	c := &getDisk{
		clientConfig: clientConfig,
		serverConfig: serverConfig,
	}

	cmd := &cobra.Command{
		Use: "get",
		RunE: func(cmd *cobra.Command, args []string) error {
			return c.run()
		},
	}

	cmd.Flags().StringVar(&c.diskID, "id", "", "ID of disk to get; required")
	if err := cmd.MarkFlagRequired("id"); err != nil {
		log.Fatalf("Error setting flag id as required: %v", err)
	}

	return cmd
}

////////////////////////////////////////////////////////////////////////////////

type listDisks struct {
	clientConfig *client_config.ClientConfig
	serverConfig *server_config.ServerConfig

	folderID string
}

func (c *listDisks) run() error {
	ctx := createContextWithoutAuthHeader(c.clientConfig)

	resourceStorage, db, err := createResourceStorage(ctx, c.serverConfig)
	if err != nil {
		return err
	}
	defer db.Close(ctx)

	ids, err := resourceStorage.ListDisks(ctx, c.folderID)
	if err != nil {
		return err
	}

	fmt.Println(strings.Join(ids, "\n"))

	return nil
}

func createListDisksCmd(
	clientConfig *client_config.ClientConfig,
	serverConfig *server_config.ServerConfig,
) *cobra.Command {

	c := &listDisks{
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
		"ID of folder where disks are located; optional",
	)
	return cmd
}

////////////////////////////////////////////////////////////////////////////////

type createDisk struct {
	clientConfig    *client_config.ClientConfig
	srcImageID      string
	srcSnapshotID   string
	size            int64
	zoneID          string
	diskID          string
	blockSize       int64
	kind            diskKind
	cloudID         string
	folderID        string
	storagePoolName string
	agentIds        agentIds
}

func (c *createDisk) run(clientConfig *client_config.ClientConfig) error {
	ctx, err := createContext(clientConfig)
	if err != nil {
		return fmt.Errorf("failed to create context: %w", err)
	}

	client, err := internal_client.NewClient(ctx, clientConfig)
	if err != nil {
		return fmt.Errorf("failed to create client: %w", err)
	}
	defer client.Close()

	req := &disk_manager.CreateDiskRequest{
		Size: c.size,
		DiskId: &disk_manager.DiskId{
			ZoneId: c.zoneID,
			DiskId: c.diskID,
		},
		BlockSize:       c.blockSize,
		Kind:            disk_manager.DiskKind(c.kind),
		CloudId:         c.cloudID,
		FolderId:        c.folderID,
		StoragePoolName: c.storagePoolName,
		AgentIds:        c.agentIds,
	}

	if c.srcImageID != "" {
		req.Src = &disk_manager.CreateDiskRequest_SrcImageId{
			SrcImageId: c.srcImageID,
		}
	} else if c.srcSnapshotID != "" {
		req.Src = &disk_manager.CreateDiskRequest_SrcSnapshotId{
			SrcSnapshotId: c.srcSnapshotID,
		}
	} else {
		req.Src = &disk_manager.CreateDiskRequest_SrcEmpty{
			SrcEmpty: &empty.Empty{},
		}
	}

	resp, err := client.CreateDisk(getRequestContext(ctx), req)
	if err != nil {
		return err
	}

	fmt.Printf("Operation: %v\n", resp.Id)

	return internal_client.WaitOperation(ctx, client, resp.Id)
}

func createCreateDiskCmd(clientConfig *client_config.ClientConfig) *cobra.Command {
	c := &createDisk{
		clientConfig: clientConfig,
		kind:         diskKind(disk_manager.DiskKind_DISK_KIND_SSD),
	}

	cmd := &cobra.Command{
		Use: "create",
		RunE: func(cmd *cobra.Command, args []string) error {
			return c.run(clientConfig)
		},
	}

	cmd.Flags().StringVar(&c.srcImageID, "src-image-id", "", "ID of image to create disk from")
	cmd.Flags().StringVar(&c.srcSnapshotID, "src-snapshot-id", "", "ID of snapshot to create disk from")

	cmd.Flags().Int64Var(&c.size, "size", 0, "size of disk in bytes; required")
	if err := cmd.MarkFlagRequired("size"); err != nil {
		log.Fatalf("Error setting flag size as required: %v", err)
	}

	cmd.Flags().StringVar(&c.zoneID, "zone-id", "", "zone ID in which to create disk; required")
	if err := cmd.MarkFlagRequired("zone-id"); err != nil {
		log.Fatalf("Error setting flag zone-id as required: %v", err)
	}

	cmd.Flags().StringVar(&c.diskID, "id", "", "disk ID; required")
	if err := cmd.MarkFlagRequired("id"); err != nil {
		log.Fatalf("Error setting flag id as required: %v", err)
	}

	cmd.Flags().Int64Var(&c.blockSize, "block-size", 0, "block size in bytes. 0 - use default")
	cmd.Flags().Var(&c.kind, "kind", "disk kind")

	cmd.Flags().StringVar(&c.cloudID, "cloud-id", "", "cloud ID of the disk owner; required")
	if err := cmd.MarkFlagRequired("cloud-id"); err != nil {
		log.Fatalf("Error setting flag cloud-id as required: %v", err)
	}

	cmd.Flags().StringVar(&c.folderID, "folder-id", "", "folder ID of the disk owner; required")
	if err := cmd.MarkFlagRequired("folder-id"); err != nil {
		log.Fatalf("Error setting flag folder-id as required: %v", err)
	}

	cmd.Flags().StringVar(&c.storagePoolName, "storage-pool-name", "", "storage pool name")
	cmd.Flags().Var(&c.agentIds, "agent-id", "agent id (several agents can be added at a time)")

	return cmd
}

////////////////////////////////////////////////////////////////////////////////

type deleteDisk struct {
	clientConfig *client_config.ClientConfig
	zoneID       string
	diskID       string

	dontDestroy bool
}

func (c *deleteDisk) run(clientConfig *client_config.ClientConfig) error {
	ctx, err := createContext(clientConfig)
	if err != nil {
		return fmt.Errorf("failed to create context: %w", err)
	}

	err = requestDeletionConfirmation("disk", c.diskID)
	if err != nil {
		return err
	}

	client, err := internal_client.NewClient(ctx, clientConfig)
	if err != nil {
		return fmt.Errorf("failed to create client: %w", err)
	}
	defer client.Close()

	req := &disk_manager.DeleteDiskRequest{
		DiskId: &disk_manager.DiskId{
			ZoneId: c.zoneID,
			DiskId: c.diskID,
		},
	}

	resp, err := client.DeleteDisk(getRequestContext(ctx), req)
	if err != nil {
		return err
	}

	fmt.Printf("Operation: %v\n", resp.Id)

	return internal_client.WaitOperation(ctx, client, resp.Id)
}

func createDeleteDiskCmd(clientConfig *client_config.ClientConfig) *cobra.Command {
	c := &deleteDisk{
		clientConfig: clientConfig,
	}

	cmd := &cobra.Command{
		Use: "delete",
		RunE: func(cmd *cobra.Command, args []string) error {
			return c.run(clientConfig)
		},
	}

	cmd.Flags().StringVar(&c.zoneID, "zone-id", "", "zone ID where disk is located; required")
	if err := cmd.MarkFlagRequired("zone-id"); err != nil {
		log.Fatalf("Error setting flag zone-id as required: %v", err)
	}

	cmd.Flags().StringVar(&c.diskID, "id", "", "disk ID; required")
	if err := cmd.MarkFlagRequired("id"); err != nil {
		log.Fatalf("Error setting flag id as required: %v", err)
	}

	return cmd
}

////////////////////////////////////////////////////////////////////////////////

type resizeDisk struct {
	clientConfig *client_config.ClientConfig
	zoneID       string
	diskID       string
	size         int64
}

func (c *resizeDisk) run(clientConfig *client_config.ClientConfig) error {
	ctx, err := createContext(clientConfig)
	if err != nil {
		return fmt.Errorf("failed to create context: %w", err)
	}

	client, err := internal_client.NewClient(ctx, clientConfig)
	if err != nil {
		return fmt.Errorf("failed to create client: %w", err)
	}
	defer client.Close()

	resp, err := client.ResizeDisk(getRequestContext(ctx), &disk_manager.ResizeDiskRequest{
		DiskId: &disk_manager.DiskId{
			ZoneId: c.zoneID,
			DiskId: c.diskID,
		},
		Size: c.size,
	})
	if err != nil {
		return err
	}

	fmt.Printf("Operation: %v\n", resp.Id)

	return internal_client.WaitOperation(ctx, client, resp.Id)
}

////////////////////////////////////////////////////////////////////////////////

func createResizeDiskCmd(clientConfig *client_config.ClientConfig) *cobra.Command {
	c := &resizeDisk{
		clientConfig: clientConfig,
	}

	cmd := &cobra.Command{
		Use: "resize",
		RunE: func(cmd *cobra.Command, args []string) error {
			return c.run(clientConfig)
		},
	}

	cmd.Flags().StringVar(&c.zoneID, "zone-id", "", "zone ID where disk is located; required")
	if err := cmd.MarkFlagRequired("zone-id"); err != nil {
		log.Fatalf("Error setting flag zone-id as required: %v", err)
	}

	cmd.Flags().StringVar(&c.diskID, "id", "", "disk ID; required")
	if err := cmd.MarkFlagRequired("id"); err != nil {
		log.Fatalf("Error setting flag id as required: %v", err)
	}

	cmd.Flags().Int64Var(&c.size, "size", 0, "size of disk in bytes (after resize); required")
	if err := cmd.MarkFlagRequired("size"); err != nil {
		log.Fatalf("Error setting flag size as required: %v", err)
	}

	return cmd
}

////////////////////////////////////////////////////////////////////////////////

type alterDisk struct {
	clientConfig *client_config.ClientConfig
	zoneID       string
	diskID       string
	cloudID      string
	folderID     string
}

func (c *alterDisk) run(clientConfig *client_config.ClientConfig) error {
	ctx, err := createContext(clientConfig)
	if err != nil {
		return fmt.Errorf("failed to create context: %w", err)
	}

	client, err := internal_client.NewClient(ctx, clientConfig)
	if err != nil {
		return fmt.Errorf("failed to create client: %w", err)
	}
	defer client.Close()

	resp, err := client.AlterDisk(getRequestContext(ctx), &disk_manager.AlterDiskRequest{
		DiskId: &disk_manager.DiskId{
			ZoneId: c.zoneID,
			DiskId: c.diskID,
		},
		CloudId:  c.cloudID,
		FolderId: c.folderID,
	})
	if err != nil {
		return err
	}

	fmt.Printf("Operation: %v\n", resp.Id)

	return internal_client.WaitOperation(ctx, client, resp.Id)
}

////////////////////////////////////////////////////////////////////////////////

func createAlterDiskCmd(clientConfig *client_config.ClientConfig) *cobra.Command {
	c := &alterDisk{
		clientConfig: clientConfig,
	}

	cmd := &cobra.Command{
		Use: "alter",
		RunE: func(cmd *cobra.Command, args []string) error {
			return c.run(clientConfig)
		},
	}

	cmd.Flags().StringVar(&c.zoneID, "zone-id", "", "zone ID where disk is located; required")
	if err := cmd.MarkFlagRequired("zone-id"); err != nil {
		log.Fatalf("Error setting flag zone-id as required: %v", err)
	}

	cmd.Flags().StringVar(&c.diskID, "id", "", "disk ID; required")
	if err := cmd.MarkFlagRequired("id"); err != nil {
		log.Fatalf("Error setting flag id as required: %v", err)
	}

	cmd.Flags().StringVar(&c.cloudID, "cloud-id", "", "cloud ID of the disk owner; required")
	if err := cmd.MarkFlagRequired("cloud-id"); err != nil {
		log.Fatalf("Error setting flag cloud-id as required: %v", err)
	}

	cmd.Flags().StringVar(&c.folderID, "folder-id", "", "folder ID of the disk owner; required")
	if err := cmd.MarkFlagRequired("folder-id"); err != nil {
		log.Fatalf("Error setting flag folder-id as required: %v", err)
	}

	return cmd
}

////////////////////////////////////////////////////////////////////////////////

type assignDisk struct {
	clientConfig *client_config.ClientConfig
	zoneID       string
	diskID       string
	instanceID   string
	host         string
	token        string
}

func (c *assignDisk) run(clientConfig *client_config.ClientConfig) error {
	ctx, err := createContext(clientConfig)
	if err != nil {
		return fmt.Errorf("failed to create context: %w", err)
	}

	client, err := internal_client.NewClient(ctx, clientConfig)
	if err != nil {
		return fmt.Errorf("failed to create client: %w", err)
	}
	defer client.Close()

	resp, err := client.AssignDisk(getRequestContext(ctx), &disk_manager.AssignDiskRequest{
		DiskId: &disk_manager.DiskId{
			ZoneId: c.zoneID,
			DiskId: c.diskID,
		},
		InstanceId: c.instanceID,
		Host:       c.host,
		Token:      c.token,
	})
	if err != nil {
		return err
	}

	fmt.Printf("Operation: %v\n", resp.Id)

	return internal_client.WaitOperation(ctx, client, resp.Id)
}

////////////////////////////////////////////////////////////////////////////////

func createAssignDiskCmd(clientConfig *client_config.ClientConfig) *cobra.Command {
	c := &assignDisk{
		clientConfig: clientConfig,
	}

	cmd := &cobra.Command{
		Use: "assign",
		RunE: func(cmd *cobra.Command, args []string) error {
			return c.run(clientConfig)
		},
	}

	cmd.Flags().StringVar(&c.zoneID, "zone-id", "", "zone ID where disk is located; required")
	if err := cmd.MarkFlagRequired("zone-id"); err != nil {
		log.Fatalf("Error setting flag zone-id as required: %v", err)
	}

	cmd.Flags().StringVar(&c.diskID, "id", "", "disk ID; required")
	if err := cmd.MarkFlagRequired("id"); err != nil {
		log.Fatalf("Error setting flag id as required: %v", err)
	}

	cmd.Flags().StringVar(&c.instanceID, "instance-id", "", "id of instance to assign disk to; required")
	if err := cmd.MarkFlagRequired("instance-id"); err != nil {
		log.Fatalf("Error setting flag instance-id as required: %v", err)
	}

	cmd.Flags().StringVar(&c.host, "host", "", "host where instance is located; required")
	if err := cmd.MarkFlagRequired("host"); err != nil {
		log.Fatalf("Error setting flag host as required: %v", err)
	}

	cmd.Flags().StringVar(&c.token, "token", "", "token; required")
	if err := cmd.MarkFlagRequired("token"); err != nil {
		log.Fatalf("Error setting flag token as required: %v", err)
	}

	return cmd
}

////////////////////////////////////////////////////////////////////////////////

type unassignDisk struct {
	clientConfig *client_config.ClientConfig
	zoneID       string
	diskID       string
}

func (c *unassignDisk) run(clientConfig *client_config.ClientConfig) error {
	ctx, err := createContext(clientConfig)
	if err != nil {
		return fmt.Errorf("failed to create context: %w", err)
	}

	client, err := internal_client.NewClient(ctx, clientConfig)
	if err != nil {
		return fmt.Errorf("failed to create client: %w", err)
	}
	defer client.Close()

	resp, err := client.UnassignDisk(getRequestContext(ctx), &disk_manager.UnassignDiskRequest{
		DiskId: &disk_manager.DiskId{
			ZoneId: c.zoneID,
			DiskId: c.diskID,
		},
	})
	if err != nil {
		return err
	}

	fmt.Printf("Operation: %v\n", resp.Id)

	return internal_client.WaitOperation(ctx, client, resp.Id)
}

////////////////////////////////////////////////////////////////////////////////

func createUnassignDiskCmd(clientConfig *client_config.ClientConfig) *cobra.Command {
	c := &unassignDisk{
		clientConfig: clientConfig,
	}

	cmd := &cobra.Command{
		Use: "unassign",
		RunE: func(cmd *cobra.Command, args []string) error {
			return c.run(clientConfig)
		},
	}

	cmd.Flags().StringVar(&c.zoneID, "zone-id", "", "zone ID where disk is located; required")
	if err := cmd.MarkFlagRequired("zone-id"); err != nil {
		log.Fatalf("Error setting flag zone-id as required: %v", err)
	}

	cmd.Flags().StringVar(&c.diskID, "id", "", "disk ID; required")
	if err := cmd.MarkFlagRequired("id"); err != nil {
		log.Fatalf("Error setting flag id as required: %v", err)
	}

	return cmd
}

////////////////////////////////////////////////////////////////////////////////

func createDisksCmd(
	clientConfig *client_config.ClientConfig,
	serverConfig *server_config.ServerConfig,
) *cobra.Command {

	cmd := &cobra.Command{
		Use: "disks",
	}

	cmd.AddCommand(
		createGetDiskCmd(clientConfig, serverConfig),
		createListDisksCmd(clientConfig, serverConfig),
		createCreateDiskCmd(clientConfig),
		createDeleteDiskCmd(clientConfig),
		createResizeDiskCmd(clientConfig),
		createAlterDiskCmd(clientConfig),
		createAssignDiskCmd(clientConfig),
		createUnassignDiskCmd(clientConfig),
	)

	return cmd
}
