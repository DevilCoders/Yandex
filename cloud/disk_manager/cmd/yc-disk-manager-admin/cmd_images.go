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

type getImage struct {
	clientConfig *client_config.ClientConfig
	serverConfig *server_config.ServerConfig

	imageID string
}

func (c *getImage) run() error {
	ctx := createContextWithoutAuthHeader(c.clientConfig)

	resourceStorage, db, err := createResourceStorage(ctx, c.serverConfig)
	if err != nil {
		return err
	}
	defer db.Close(ctx)

	image, err := resourceStorage.GetImageMeta(ctx, c.imageID)
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

func createGetImageCmd(
	clientConfig *client_config.ClientConfig,
	serverConfig *server_config.ServerConfig,
) *cobra.Command {

	c := &getImage{
		clientConfig: clientConfig,
		serverConfig: serverConfig,
	}

	cmd := &cobra.Command{
		Use: "get",
		RunE: func(cmd *cobra.Command, args []string) error {
			return c.run()
		},
	}

	cmd.Flags().StringVar(&c.imageID, "id", "", "ID of image to get; required")
	if err := cmd.MarkFlagRequired("id"); err != nil {
		log.Fatalf("Error setting flag id as required: %v", err)
	}

	return cmd
}

////////////////////////////////////////////////////////////////////////////////

type listImages struct {
	clientConfig *client_config.ClientConfig
	serverConfig *server_config.ServerConfig

	folderID string
}

func (c *listImages) run() error {
	ctx := createContextWithoutAuthHeader(c.clientConfig)

	resourceStorage, db, err := createResourceStorage(ctx, c.serverConfig)
	if err != nil {
		return err
	}
	defer db.Close(ctx)

	ids, err := resourceStorage.ListImages(ctx, c.folderID)
	if err != nil {
		return err
	}

	fmt.Println(strings.Join(ids, "\n"))

	return nil
}

func createListImagesCmd(
	clientConfig *client_config.ClientConfig,
	serverConfig *server_config.ServerConfig,
) *cobra.Command {

	c := &listImages{
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
		"ID of folder where images are located; optional",
	)
	return cmd
}

////////////////////////////////////////////////////////////////////////////////

type createImage struct {
	clientConfig  *client_config.ClientConfig
	srcURL        string
	format        string
	srcImageID    string
	srcSnapshotID string
	dstImageID    string
	folderID      string
}

func (c *createImage) run(clientConfig *client_config.ClientConfig) error {
	ctx, err := createContext(clientConfig)
	if err != nil {
		return fmt.Errorf("failed to create context: %w", err)
	}

	client, err := internal_client.NewClient(ctx, clientConfig)
	if err != nil {
		return fmt.Errorf("failed to create client: %w", err)
	}
	defer client.Close()

	req := &disk_manager.CreateImageRequest{
		DstImageId: c.dstImageID,
		FolderId:   c.folderID,
	}

	if c.srcURL != "" {
		req.Src = &disk_manager.CreateImageRequest_SrcUrl{
			SrcUrl: &disk_manager.ImageUrl{
				Url:    c.srcURL,
				Format: c.format,
			},
		}
	} else if c.srcImageID != "" {
		req.Src = &disk_manager.CreateImageRequest_SrcImageId{
			SrcImageId: c.srcImageID,
		}
	} else if c.srcSnapshotID != "" {
		req.Src = &disk_manager.CreateImageRequest_SrcSnapshotId{
			SrcSnapshotId: c.srcSnapshotID,
		}
	} else {
		return fmt.Errorf("one of src-url or src-image-id should be defined")
	}

	resp, err := client.CreateImage(getRequestContext(ctx), req)
	if err != nil {
		return err
	}

	fmt.Printf("Operation: %v\n", resp.Id)

	return internal_client.WaitOperation(ctx, client, resp.Id)
}

func createCreateImageCmd(clientConfig *client_config.ClientConfig) *cobra.Command {
	c := &createImage{
		clientConfig: clientConfig,
	}

	cmd := &cobra.Command{
		Use: "create",
		RunE: func(cmd *cobra.Command, args []string) error {
			return c.run(clientConfig)
		},
	}

	cmd.Flags().StringVar(&c.srcURL, "src-url", "", "URL to create image from")
	cmd.Flags().StringVar(&c.format, "format", "raw", "Format of image to create")
	cmd.Flags().StringVar(&c.srcImageID, "src-image-id", "", "ID of image to create image from")
	cmd.Flags().StringVar(&c.srcSnapshotID, "src-snapshot-id", "", "ID of snapshot to create image from")

	cmd.Flags().StringVar(&c.dstImageID, "id", "", "ID of image to create; required")
	if err := cmd.MarkFlagRequired("id"); err != nil {
		log.Fatalf("Error setting flag id as required: %v", err)
	}

	cmd.Flags().StringVar(&c.folderID, "folder-id", "", "folder ID of the image owner; required")
	if err := cmd.MarkFlagRequired("folder-id"); err != nil {
		log.Fatalf("Error setting flag folder-id as required: %v", err)
	}

	return cmd
}

////////////////////////////////////////////////////////////////////////////////

type deleteImage struct {
	clientConfig *client_config.ClientConfig
	imageID      string
}

func (c *deleteImage) run(clientConfig *client_config.ClientConfig) error {
	ctx, err := createContext(clientConfig)
	if err != nil {
		return fmt.Errorf("failed to create context: %w", err)
	}

	err = requestDeletionConfirmation("image", c.imageID)
	if err != nil {
		return err
	}

	client, err := internal_client.NewClient(ctx, clientConfig)
	if err != nil {
		return fmt.Errorf("failed to create client: %w", err)
	}
	defer client.Close()

	req := &disk_manager.DeleteImageRequest{
		ImageId: c.imageID,
	}

	resp, err := client.DeleteImage(getRequestContext(ctx), req)
	if err != nil {
		return err
	}

	fmt.Printf("Operation: %v\n", resp.Id)

	return internal_client.WaitOperation(ctx, client, resp.Id)
}

func createDeleteImageCmd(clientConfig *client_config.ClientConfig) *cobra.Command {
	c := &deleteImage{
		clientConfig: clientConfig,
	}

	cmd := &cobra.Command{
		Use: "delete",
		RunE: func(cmd *cobra.Command, args []string) error {
			return c.run(clientConfig)
		},
	}

	cmd.Flags().StringVar(&c.imageID, "id", "", "ID of image to delete; required")
	if err := cmd.MarkFlagRequired("id"); err != nil {
		log.Fatalf("Error setting flag id as required: %v", err)
	}

	return cmd
}

////////////////////////////////////////////////////////////////////////////////

func createImagesCmd(
	clientConfig *client_config.ClientConfig,
	serverConfig *server_config.ServerConfig,
) *cobra.Command {
	cmd := &cobra.Command{
		Use: "images",
	}

	cmd.AddCommand(
		createGetImageCmd(clientConfig, serverConfig),
		createListImagesCmd(clientConfig, serverConfig),
		createCreateImageCmd(clientConfig),
		createDeleteImageCmd(clientConfig),
	)

	return cmd
}
