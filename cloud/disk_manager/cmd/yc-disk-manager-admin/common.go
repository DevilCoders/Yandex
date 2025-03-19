package main

import (
	"bufio"
	"context"
	"fmt"
	"log"
	"os"

	"github.com/gofrs/uuid"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/auth"
	client_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/configs/client/config"
	server_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/configs/server/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring/metrics"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	tasks_storage "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/storage"
)

////////////////////////////////////////////////////////////////////////////////

var curLaunchID string
var lastReqNumber int

////////////////////////////////////////////////////////////////////////////////

func createContextWithoutAuthHeader(
	config *client_config.ClientConfig,
) context.Context {

	ctx := context.Background()
	logger := logging.CreateLogger(config.LoggingConfig)
	ctx = logging.SetLogger(ctx, logger)
	return ctx
}

func createContext(
	config *client_config.ClientConfig,
) (context.Context, error) {

	ctx := createContextWithoutAuthHeader(config)

	var err error
	if !config.GetDisableAuthentication() {
		ctx, err = addAuthHeader(ctx, config)
		if err != nil {
			return nil, err
		}
	}

	return ctx, nil
}

////////////////////////////////////////////////////////////////////////////////

func generateID() string {
	return fmt.Sprintf(
		"%v_%v",
		"yc-disk-manager-admin",
		uuid.Must(uuid.NewV4()).String(),
	)
}

func getRequestContext(ctx context.Context) context.Context {
	if len(curLaunchID) == 0 {
		curLaunchID = generateID()
	}

	lastReqNumber++

	cookie := fmt.Sprintf("%v_%v", curLaunchID, lastReqNumber)
	ctx = headers.SetOutgoingIdempotencyKey(ctx, cookie)
	ctx = headers.SetOutgoingRequestID(ctx, cookie)
	return ctx
}

func requestDeletionConfirmation(objectType string, objectID string) error {
	log.Printf("confirm destruction by typing %s id to stdin", objectType)
	scanner := bufio.NewScanner(os.Stdin)
	scanner.Scan()
	if scanner.Text() != objectID {
		return fmt.Errorf(
			"confirmation failed: %s != %s",
			scanner.Text(),
			objectID)
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////

func createYDBClient(
	ctx context.Context,
	config *server_config.ServerConfig,
) (*persistence.YDBClient, error) {

	creds := auth.NewCredentials(ctx, config.AuthConfig)

	db, err := persistence.CreateYDBClient(
		ctx,
		config.PersistenceConfig,
		persistence.WithCredentials(creds),
	)
	if err != nil {
		return nil, fmt.Errorf("failed to connect to DB: %w", err)
	}

	return db, nil
}

func createTaskStorage(
	ctx context.Context,
	config *server_config.ServerConfig,
) (tasks_storage.Storage, *persistence.YDBClient, error) {

	db, err := createYDBClient(ctx, config)
	if err != nil {
		return nil, nil, err
	}

	taskStorage, err := tasks_storage.CreateStorage(
		config.TasksConfig,
		metrics.CreateEmptyRegistry(),
		db,
	)

	return taskStorage, db, err
}

func createResourceStorage(
	ctx context.Context,
	config *server_config.ServerConfig,
) (resources.Storage, *persistence.YDBClient, error) {

	db, err := createYDBClient(ctx, config)
	if err != nil {
		return nil, nil, err
	}

	resourcesStorage, err := resources.CreateStorage(
		config.GetDisksConfig().GetStorageFolder(),
		config.GetImagesConfig().GetStorageFolder(),
		config.GetSnapshotsConfig().GetStorageFolder(),
		config.GetFilesystemConfig().GetStorageFolder(),
		config.GetPlacementGroupConfig().GetStorageFolder(),
		db,
	)

	return resourcesStorage, db, err
}
