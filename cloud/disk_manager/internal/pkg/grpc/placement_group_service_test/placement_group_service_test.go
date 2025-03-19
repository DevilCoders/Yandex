package tests

import (
	"os"
	"testing"

	"github.com/golang/protobuf/proto"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	internal_client "a.yandex-team.ru/cloud/disk_manager/internal/pkg/client"
	node_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/configs/server/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/grpc/testcommon"
	ydb "a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
	db "a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
)

////////////////////////////////////////////////////////////////////////////////

func TestDiskServiceCreateDeletePlacementGroup(t *testing.T) {
	ctx := testcommon.CreateContext()

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	configString := os.Getenv("DISK_MANAGER_RECIPE_SERVER_CONFIG")

	require.NotEmpty(t, configString)

	var config node_config.ServerConfig
	err = proto.UnmarshalText(configString, &config)
	require.NoError(t, err)

	ydbClient, err := ydb.CreateYDBClient(ctx, config.GetPersistenceConfig())
	require.NoError(t, err)

	storage, err := db.CreateStorage(
		"",
		"",
		"",
		"",
		config.GetPlacementGroupConfig().GetStorageFolder(),
		ydbClient,
	)
	require.NoError(t, err)

	groupID := t.Name()

	reqCtx := testcommon.GetRequestContext(t, ctx)
	operation, err := client.CreatePlacementGroup(reqCtx, &disk_manager.CreatePlacementGroupRequest{
		GroupId: &disk_manager.GroupId{
			ZoneId:  "zone",
			GroupId: groupID,
		},
		PlacementStrategy: disk_manager.PlacementStrategy_PLACEMENT_STRATEGY_SPREAD,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	_, err = storage.CheckPlacementGroupReady(ctx, groupID)
	require.NoError(t, err)

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.DeletePlacementGroup(reqCtx, &disk_manager.DeletePlacementGroupRequest{
		GroupId: &disk_manager.GroupId{
			ZoneId:  "zone",
			GroupId: groupID,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)
}
