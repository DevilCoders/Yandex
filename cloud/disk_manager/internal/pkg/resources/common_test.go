package resources

import (
	"context"
	"fmt"
	"os"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
	persistence_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence/config"
)

////////////////////////////////////////////////////////////////////////////////

func createContext() context.Context {
	return logging.SetLogger(
		context.Background(),
		logging.CreateStderrLogger(logging.DebugLevel),
	)
}

func createYDB(ctx context.Context) (*persistence.YDBClient, error) {
	endpoint := fmt.Sprintf(
		"localhost:%v",
		os.Getenv("DISK_MANAGER_RECIPE_KIKIMR_PORT"),
	)
	database := "/Root"
	rootPath := "disk_manager"

	return persistence.CreateYDBClient(
		ctx,
		&persistence_config.PersistenceConfig{
			Endpoint: &endpoint,
			Database: &database,
			RootPath: &rootPath,
		},
	)
}

func createStorage(
	t *testing.T,
	ctx context.Context,
	db *persistence.YDBClient,
) Storage {

	disksFolder := fmt.Sprintf("%v/disks", t.Name())
	imagesFolder := fmt.Sprintf("%v/images", t.Name())
	snapshotsFolder := fmt.Sprintf("%v/snapshots", t.Name())
	filesystemsFolder := fmt.Sprintf("%v/filesystems", t.Name())
	placementGroupsFolder := fmt.Sprintf("%v/placement_groups", t.Name())

	err := CreateYDBTables(
		ctx,
		disksFolder,
		imagesFolder,
		snapshotsFolder,
		filesystemsFolder,
		placementGroupsFolder,
		db,
	)
	require.NoError(t, err)

	storage, err := CreateStorage(
		disksFolder,
		imagesFolder,
		snapshotsFolder,
		filesystemsFolder,
		placementGroupsFolder,
		db,
	)
	require.NoError(t, err)

	return storage
}
