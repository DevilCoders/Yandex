package resources

import (
	"context"
	"fmt"

	ydb_table "github.com/ydb-platform/ydb-go-sdk/v3/table"
	ydb_result "github.com/ydb-platform/ydb-go-sdk/v3/table/result"
	ydb_named "github.com/ydb-platform/ydb-go-sdk/v3/table/result/named"
	ydb_types "github.com/ydb-platform/ydb-go-sdk/v3/table/types"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

type storageYDB struct {
	db                  *persistence.YDBClient
	disksPath           string
	imagesPath          string
	snapshotsPath       string
	filesystemsPath     string
	placementGroupsPath string
}

////////////////////////////////////////////////////////////////////////////////

func CreateStorage(
	disksFolder string,
	imagesFolder string,
	snapshotsFolder string,
	filesystemsFolder string,
	placementGroupsPath string,
	db *persistence.YDBClient,
) (Storage, error) {

	return &storageYDB{
		db:                  db,
		disksPath:           db.AbsolutePath(disksFolder),
		imagesPath:          db.AbsolutePath(imagesFolder),
		snapshotsPath:       db.AbsolutePath(snapshotsFolder),
		filesystemsPath:     db.AbsolutePath(filesystemsFolder),
		placementGroupsPath: db.AbsolutePath(placementGroupsPath),
	}, nil
}

////////////////////////////////////////////////////////////////////////////////

func CreateYDBTables(
	ctx context.Context,
	disksFolder string,
	imagesFolder string,
	snapshotsFolder string,
	filesystemsFolder string,
	placementGroupsFolder string,
	db *persistence.YDBClient,
) error {

	err := createDisksYDBTables(ctx, disksFolder, db)
	if err != nil {
		return err
	}

	err = createImagesYDBTables(ctx, imagesFolder, db)
	if err != nil {
		return err
	}

	err = createSnapshotsYDBTables(ctx, snapshotsFolder, db)
	if err != nil {
		return err
	}

	if filesystemsFolder != "" {
		err = createFilesystemsYDBTables(ctx, filesystemsFolder, db)
		if err != nil {
			return err
		}
	}

	err = createPlacementGroupsYDBTables(ctx, placementGroupsFolder, db)
	if err != nil {
		return err
	}

	return nil
}

func DropYDBTables(
	ctx context.Context,
	disksFolder string,
	imagesFolder string,
	snapshotsFolder string,
	filesystemsFolder string,
	placementGroupsFolder string,
	db *persistence.YDBClient,
) error {

	err := dropDisksYDBTables(ctx, disksFolder, db)
	if err != nil {
		return err
	}

	err = dropImagesYDBTables(ctx, imagesFolder, db)
	if err != nil {
		return err
	}

	err = dropSnapshotsYDBTables(ctx, snapshotsFolder, db)
	if err != nil {
		return err
	}

	if filesystemsFolder != "" {
		err = dropFilesystemsYDBTables(ctx, filesystemsFolder, db)
		if err != nil {
			return err
		}
	}

	err = dropPlacementGroupsYDBTables(ctx, placementGroupsFolder, db)
	if err != nil {
		return err
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////

func scanDiskState(res ydb_result.Result) (state diskState, err error) {
	err = res.ScanNamed(
		ydb_named.OptionalWithDefault("id", &state.id),
		ydb_named.OptionalWithDefault("zone_id", &state.zoneID),
		ydb_named.OptionalWithDefault("src_image_id", &state.srcImageID),
		ydb_named.OptionalWithDefault("src_snapshot_id", &state.srcSnapshotID),
		ydb_named.OptionalWithDefault("blocks_count", &state.blocksCount),
		ydb_named.OptionalWithDefault("block_size", &state.blockSize),
		ydb_named.OptionalWithDefault("kind", &state.kind),
		ydb_named.OptionalWithDefault("cloud_id", &state.cloudID),
		ydb_named.OptionalWithDefault("folder_id", &state.folderID),
		ydb_named.OptionalWithDefault("placement_group_id", &state.placementGroupID),
		ydb_named.OptionalWithDefault("base_disk_id", &state.baseDiskID),
		ydb_named.OptionalWithDefault("base_disk_checkpoint_id", &state.baseDiskCheckpointID),
		ydb_named.OptionalWithDefault("create_request", &state.createRequest),
		ydb_named.OptionalWithDefault("create_task_id", &state.createTaskID),
		ydb_named.OptionalWithDefault("creating_at", &state.creatingAt),
		ydb_named.OptionalWithDefault("created_at", &state.createdAt),
		ydb_named.OptionalWithDefault("created_by", &state.createdBy),
		ydb_named.OptionalWithDefault("delete_task_id", &state.deleteTaskID),
		ydb_named.OptionalWithDefault("deleting_at", &state.deletingAt),
		ydb_named.OptionalWithDefault("deleted_at", &state.deletedAt),
		ydb_named.OptionalWithDefault("status", &state.status),
	)
	if err != nil {
		return state, &errors.NonRetriableError{
			Err: fmt.Errorf("scanDiskStates: failed to parse row: %w", err),
		}
	}
	return state, res.Err()
}

////////////////////////////////////////////////////////////////////////////////

func listResources(
	ctx context.Context,
	session *persistence.Session,
	tablesPath string,
	tableName string,
	folderID string,
) ([]string, error) {

	var (
		res ydb_result.Result
		err error
	)
	if len(folderID) == 0 {
		_, res, err = session.ExecuteRO(ctx, fmt.Sprintf(`
			--!syntax_v1
			pragma TablePathPrefix = "%v";

			select id from %v
		`, tablesPath, tableName), nil)
	} else {
		_, res, err = session.ExecuteRO(ctx, fmt.Sprintf(`
			--!syntax_v1
			pragma TablePathPrefix = "%v";
			declare $folder_id as Utf8;

			select id
			from %v
			where folder_id = $folder_id
		`, tablesPath, tableName), ydb_table.NewQueryParameters(
			ydb_table.ValueParam("$folder_id", ydb_types.UTF8Value(folderID)),
		))
	}
	if err != nil {
		return nil, err
	}
	defer res.Close()

	var ids []string

	for res.NextResultSet(ctx) {
		for res.NextRow() {
			var id string
			err = res.ScanNamed(
				ydb_named.OptionalWithDefault("id", &id),
			)
			if err != nil {
				return nil, &errors.NonRetriableError{
					Err: fmt.Errorf(
						"listResources: failed to parse row from %v: %w",
						tableName,
						err,
					),
				}
			}

			ids = append(ids, id)
		}
	}

	return ids, nil
}
