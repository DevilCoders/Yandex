package resources

import (
	"bytes"
	"context"
	"fmt"
	"time"

	"github.com/golang/protobuf/proto"
	ydb_table "github.com/ydb-platform/ydb-go-sdk/v3/table"
	ydb_result "github.com/ydb-platform/ydb-go-sdk/v3/table/result"
	ydb_named "github.com/ydb-platform/ydb-go-sdk/v3/table/result/named"
	ydb_types "github.com/ydb-platform/ydb-go-sdk/v3/table/types"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

type diskStatus uint32

func (s *diskStatus) UnmarshalYDB(res ydb_types.RawValue) error {
	*s = diskStatus(res.Int64())
	return nil
}

// NOTE: These values are stored in DB, do not shuffle them around.
const (
	diskStatusCreating diskStatus = iota
	diskStatusReady    diskStatus = iota
	diskStatusDeleting diskStatus = iota
	diskStatusDeleted  diskStatus = iota
)

func diskStatusToString(status diskStatus) string {
	switch status {
	case diskStatusCreating:
		return "creating"
	case diskStatusReady:
		return "ready"
	case diskStatusDeleting:
		return "deleting"
	case diskStatusDeleted:
		return "deleted"
	}

	return fmt.Sprintf("unknown_%v", status)
}

////////////////////////////////////////////////////////////////////////////////

// This is mapped into a DB row. If you change this struct, make sure to update
// the mapping code.
type diskState struct {
	id               string
	zoneID           string
	srcImageID       string
	srcSnapshotID    string
	blocksCount      uint64
	blockSize        uint32
	kind             string
	cloudID          string
	folderID         string
	placementGroupID string

	baseDiskID           string
	baseDiskCheckpointID string

	createRequest []byte
	createTaskID  string
	creatingAt    time.Time
	createdAt     time.Time
	createdBy     string
	deleteTaskID  string
	deletingAt    time.Time
	deletedAt     time.Time

	status diskStatus
}

func (s *diskState) toDiskMeta() *DiskMeta {
	// TODO: Disk.CreateRequest should be []byte, because we can't unmarshal
	// it here, without knowing particular protobuf message type.
	return &DiskMeta{
		ID:               s.id,
		ZoneID:           s.zoneID,
		SrcImageID:       s.srcImageID,
		SrcSnapshotID:    s.srcSnapshotID,
		BlocksCount:      s.blocksCount,
		BlockSize:        s.blockSize,
		Kind:             s.kind,
		CloudID:          s.cloudID,
		FolderID:         s.folderID,
		PlacementGroupID: s.placementGroupID,

		BaseDiskID:           s.baseDiskID,
		BaseDiskCheckpointID: s.baseDiskCheckpointID,

		CreateTaskID: s.createTaskID,
		CreatingAt:   s.creatingAt,
		CreatedAt:    s.createdAt,
		CreatedBy:    s.createdBy,
		DeleteTaskID: s.deleteTaskID,
	}
}

func (s *diskState) structValue() ydb_types.Value {
	return ydb_types.StructValue(
		ydb_types.StructFieldValue("id", ydb_types.UTF8Value(s.id)),
		ydb_types.StructFieldValue("zone_id", ydb_types.UTF8Value(s.zoneID)),
		ydb_types.StructFieldValue("src_image_id", ydb_types.UTF8Value(s.srcImageID)),
		ydb_types.StructFieldValue("src_snapshot_id", ydb_types.UTF8Value(s.srcSnapshotID)),
		ydb_types.StructFieldValue("blocks_count", ydb_types.Uint64Value(s.blocksCount)),
		ydb_types.StructFieldValue("block_size", ydb_types.Uint32Value(s.blockSize)),
		ydb_types.StructFieldValue("kind", ydb_types.UTF8Value(s.kind)),
		ydb_types.StructFieldValue("cloud_id", ydb_types.UTF8Value(s.cloudID)),
		ydb_types.StructFieldValue("folder_id", ydb_types.UTF8Value(s.folderID)),
		ydb_types.StructFieldValue("placement_group_id", ydb_types.UTF8Value(s.placementGroupID)),

		ydb_types.StructFieldValue("base_disk_id", ydb_types.UTF8Value(s.baseDiskID)),
		ydb_types.StructFieldValue("base_disk_checkpoint_id", ydb_types.UTF8Value(s.baseDiskCheckpointID)),

		ydb_types.StructFieldValue("create_request", ydb_types.StringValue(s.createRequest)),
		ydb_types.StructFieldValue("create_task_id", ydb_types.UTF8Value(s.createTaskID)),
		ydb_types.StructFieldValue("creating_at", persistence.TimestampValue(s.creatingAt)),
		ydb_types.StructFieldValue("created_at", persistence.TimestampValue(s.createdAt)),
		ydb_types.StructFieldValue("created_by", ydb_types.UTF8Value(s.createdBy)),
		ydb_types.StructFieldValue("delete_task_id", ydb_types.UTF8Value(s.deleteTaskID)),
		ydb_types.StructFieldValue("deleting_at", persistence.TimestampValue(s.deletingAt)),
		ydb_types.StructFieldValue("deleted_at", persistence.TimestampValue(s.deletedAt)),

		ydb_types.StructFieldValue("status", ydb_types.Int64Value(int64(s.status))),
	)
}

func scanDiskStates(ctx context.Context, res ydb_result.Result) ([]diskState, error) {
	var states []diskState
	for res.NextResultSet(ctx) {
		for res.NextRow() {
			state, err := scanDiskState(res)
			if err != nil {
				return nil, err
			}
			states = append(states, state)
		}
	}

	return states, res.Err()
}

func diskStateStructTypeString() string {
	return `Struct<
		id: Utf8,
		zone_id: Utf8,
		src_image_id: Utf8,
		src_snapshot_id: Utf8,
		blocks_count: Uint64,
		block_size: Uint32,
		kind: Utf8,
		cloud_id: Utf8,
		folder_id: Utf8,
		placement_group_id: Utf8,

		base_disk_id: Utf8,
		base_disk_checkpoint_id: Utf8,

		create_request: String,
		create_task_id: Utf8,
		creating_at: Timestamp,
		created_at: Timestamp,
		created_by: Utf8,
		delete_task_id: Utf8,
		deleting_at: Timestamp,
		deleted_at: Timestamp,
		status: Int64>`
}

func diskStateTableDescription() persistence.CreateTableDescription {
	return persistence.MakeCreateTableDescription(
		persistence.WithColumn("id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("zone_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("src_image_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("src_snapshot_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("blocks_count", ydb_types.Optional(ydb_types.TypeUint64)),
		persistence.WithColumn("block_size", ydb_types.Optional(ydb_types.TypeUint32)),
		persistence.WithColumn("kind", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("cloud_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("folder_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("placement_group_id", ydb_types.Optional(ydb_types.TypeUTF8)),

		persistence.WithColumn("base_disk_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("base_disk_checkpoint_id", ydb_types.Optional(ydb_types.TypeUTF8)),

		persistence.WithColumn("create_request", ydb_types.Optional(ydb_types.TypeString)),
		persistence.WithColumn("create_task_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("creating_at", ydb_types.Optional(ydb_types.TypeTimestamp)),
		persistence.WithColumn("created_at", ydb_types.Optional(ydb_types.TypeTimestamp)),
		persistence.WithColumn("created_by", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("delete_task_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("deleting_at", ydb_types.Optional(ydb_types.TypeTimestamp)),
		persistence.WithColumn("deleted_at", ydb_types.Optional(ydb_types.TypeTimestamp)),

		persistence.WithColumn("status", ydb_types.Optional(ydb_types.TypeInt64)),

		persistence.WithPrimaryKeyColumn("id"),
	)
}

////////////////////////////////////////////////////////////////////////////////

func (s *storageYDB) getDisk(
	ctx context.Context,
	session *persistence.Session,
	diskID string,
) (*DiskMeta, error) {

	_, res, err := session.ExecuteRO(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $id as Utf8;

		select *
		from disks
		where id = $id
	`, s.disksPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$id", ydb_types.UTF8Value(diskID)),
	))
	if err != nil {
		return nil, err
	}
	defer res.Close()

	states, err := scanDiskStates(ctx, res)
	if err != nil {
		return nil, err
	}

	if len(states) != 0 {
		return states[0].toDiskMeta(), nil
	} else {
		return nil, nil
	}
}

func (s *storageYDB) createDisk(
	ctx context.Context,
	session *persistence.Session,
	disk DiskMeta,
) (*DiskMeta, error) {

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return nil, err
	}
	defer tx.Rollback(ctx)

	createRequest, err := proto.Marshal(disk.CreateRequest)
	if err != nil {
		return nil, &errors.NonRetriableError{
			Err: fmt.Errorf("failed to marshal create request for disk with id=%v: %w", disk.ID, err),
		}
	}

	res, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $id as Utf8;

		select *
		from disks
		where id = $id
	`, s.disksPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$id", ydb_types.UTF8Value(disk.ID)),
	))
	if err != nil {
		return nil, err
	}
	defer res.Close()

	states, err := scanDiskStates(ctx, res)
	if err != nil {
		commitErr := tx.Commit(ctx)
		if commitErr != nil {
			return nil, commitErr
		}

		return nil, err
	}

	if len(states) != 0 {
		err = tx.Commit(ctx)
		if err != nil {
			return nil, err
		}

		state := states[0]

		if state.status >= diskStatusDeleting {
			logging.Warn(ctx, "can't create already deleting/deleted disk with id=%v", disk.ID)
			return nil, &errors.NonRetriableError{
				Err: fmt.Errorf(
					"can't create already deleting/deleted disk with id=%v",
					disk.ID,
				),
				Silent: true,
			}
		}

		// Check idempotency.
		if bytes.Equal(state.createRequest, createRequest) &&
			state.createTaskID == disk.CreateTaskID &&
			state.createdBy == disk.CreatedBy {

			return state.toDiskMeta(), nil
		}

		logging.Warn(ctx, "disk with different params already exists, old=%v, new=%v", state, disk)
		return nil, nil
	}

	state := diskState{
		id:               disk.ID,
		zoneID:           disk.ZoneID,
		srcImageID:       disk.SrcImageID,
		srcSnapshotID:    disk.SrcSnapshotID,
		blocksCount:      disk.BlocksCount,
		blockSize:        disk.BlockSize,
		kind:             disk.Kind,
		cloudID:          disk.CloudID,
		folderID:         disk.FolderID,
		placementGroupID: disk.PlacementGroupID,

		baseDiskID:           disk.BaseDiskID,
		baseDiskCheckpointID: disk.BaseDiskCheckpointID,

		createRequest: createRequest,
		createTaskID:  disk.CreateTaskID,
		creatingAt:    disk.CreatingAt,
		createdBy:     disk.CreatedBy,

		status: diskStatusCreating,
	}

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $states as List<%v>;

		upsert into disks
		select *
		from AS_TABLE($states)
	`, s.disksPath, diskStateStructTypeString()), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$states", ydb_types.ListValue(state.structValue())),
	))
	if err != nil {
		return nil, err
	}

	err = tx.Commit(ctx)
	if err != nil {
		return nil, err
	}

	return state.toDiskMeta(), nil
}

func (s *storageYDB) diskCreated(
	ctx context.Context,
	session *persistence.Session,
	disk DiskMeta,
) error {

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return err
	}
	defer tx.Rollback(ctx)

	res, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $id as Utf8;

		select *
		from disks
		where id = $id
	`, s.disksPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$id", ydb_types.UTF8Value(disk.ID)),
	))
	if err != nil {
		return err
	}
	defer res.Close()

	states, err := scanDiskStates(ctx, res)
	if err != nil {
		commitErr := tx.Commit(ctx)
		if commitErr != nil {
			return commitErr
		}

		return err
	}

	if len(states) == 0 {
		err = tx.Commit(ctx)
		if err != nil {
			return err
		}

		return &errors.NonRetriableError{
			Err: fmt.Errorf("disk with id=%v is not found", disk.ID),
		}
	}

	state := states[0]

	if state.status == diskStatusReady {
		// Nothing to do.
		return tx.Commit(ctx)
	}

	if state.status != diskStatusCreating {
		err = tx.Commit(ctx)
		if err != nil {
			return err
		}

		return &errors.NonRetriableError{
			Err: fmt.Errorf(
				"disk with id=%v and status=%v can't be created",
				disk.ID,
				diskStatusToString(state.status),
			),
			Silent: true,
		}
	}

	state.status = diskStatusReady
	// Base disk id and checkpoint id become known after disk creation.
	state.baseDiskID = disk.BaseDiskID
	state.baseDiskCheckpointID = disk.BaseDiskCheckpointID
	state.createdAt = disk.CreatedAt

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $states as List<%v>;

		upsert into disks
		select *
		from AS_TABLE($states)
	`, s.disksPath, diskStateStructTypeString()), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$states", ydb_types.ListValue(state.structValue())),
	))
	if err != nil {
		return err
	}

	return tx.Commit(ctx)
}

func (s *storageYDB) deleteDisk(
	ctx context.Context,
	session *persistence.Session,
	diskID string,
	taskID string,
	deletingAt time.Time,
) (*DiskMeta, error) {

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return nil, err
	}
	defer tx.Rollback(ctx)

	res, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $id as Utf8;

		select *
		from disks
		where id = $id
	`, s.disksPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$id", ydb_types.UTF8Value(diskID)),
	))
	if err != nil {
		return nil, err
	}
	defer res.Close()

	states, err := scanDiskStates(ctx, res)
	if err != nil {
		commitErr := tx.Commit(ctx)
		if commitErr != nil {
			return nil, commitErr
		}

		return nil, err
	}

	var state diskState

	if len(states) != 0 {
		state = states[0]

		if state.status >= diskStatusDeleting {
			// Disk already marked as deleting/deleted.

			err = tx.Commit(ctx)
			if err != nil {
				return nil, err
			}

			return state.toDiskMeta(), nil
		}
	}

	state.id = diskID
	state.status = diskStatusDeleting
	state.deleteTaskID = taskID
	state.deletingAt = deletingAt

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $states as List<%v>;

		upsert into disks
		select *
		from AS_TABLE($states)
	`, s.disksPath, diskStateStructTypeString()), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$states", ydb_types.ListValue(state.structValue())),
	))
	if err != nil {
		return nil, err
	}

	err = tx.Commit(ctx)
	if err != nil {
		return nil, err
	}

	return state.toDiskMeta(), nil
}

func (s *storageYDB) diskDeleted(
	ctx context.Context,
	session *persistence.Session,
	diskID string,
	deletedAt time.Time,
) error {

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return err
	}
	defer tx.Rollback(ctx)

	res, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $id as Utf8;

		select *
		from disks
		where id = $id
	`, s.disksPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$id", ydb_types.UTF8Value(diskID)),
	))
	if err != nil {
		return err
	}
	defer res.Close()

	states, err := scanDiskStates(ctx, res)
	if err != nil {
		commitErr := tx.Commit(ctx)
		if commitErr != nil {
			return commitErr
		}

		return err
	}

	if len(states) == 0 {
		// It's possible that disk is already collected.
		return tx.Commit(ctx)
	}

	state := states[0]

	if state.status == diskStatusDeleted {
		// Nothing to do.
		return tx.Commit(ctx)
	}

	if state.status != diskStatusDeleting {
		err = tx.Commit(ctx)
		if err != nil {
			return err
		}

		return &errors.NonRetriableError{
			Err: fmt.Errorf(
				"disk with id=%v and status=%v can't be deleted",
				diskID,
				diskStatusToString(state.status),
			),
		}
	}

	state.status = diskStatusDeleted
	state.deletedAt = deletedAt

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $states as List<%v>;

		upsert into disks
		select *
		from AS_TABLE($states)
	`, s.disksPath, diskStateStructTypeString()), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$states", ydb_types.ListValue(state.structValue())),
	))
	if err != nil {
		return err
	}

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $deleted_at as Timestamp;
		declare $disk_id as Utf8;

		upsert into deleted (deleted_at, disk_id)
		values ($deleted_at, $disk_id)
	`, s.disksPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$deleted_at", persistence.TimestampValue(deletedAt)),
		ydb_table.ValueParam("$disk_id", ydb_types.UTF8Value(diskID)),
	))
	if err != nil {
		return err
	}

	return tx.Commit(ctx)
}

func (s *storageYDB) clearDeletedDisks(
	ctx context.Context,
	session *persistence.Session,
	deletedBefore time.Time,
	limit int,
) error {

	_, res, err := session.ExecuteRO(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $deleted_before as Timestamp;
		declare $limit as Uint64;

		select *
		from deleted
		where deleted_at < $deleted_before
		limit $limit
	`, s.disksPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$deleted_before", persistence.TimestampValue(deletedBefore)),
		ydb_table.ValueParam("$limit", ydb_types.Uint64Value(uint64(limit))),
	))
	if err != nil {
		return err
	}
	defer res.Close()

	for res.NextResultSet(ctx) {
		for res.NextRow() {
			var (
				deletedAt time.Time
				diskID    string
			)
			err = res.ScanNamed(
				ydb_named.OptionalWithDefault("deleted_at", &deletedAt),
				ydb_named.OptionalWithDefault("disk_id", &diskID),
			)
			if err != nil {
				return &errors.NonRetriableError{
					Err: fmt.Errorf("clearDeletedDisks: failed to parse row: %w", err),
				}
			}
			_, _, err = session.ExecuteRW(ctx, fmt.Sprintf(`
				--!syntax_v1
				pragma TablePathPrefix = "%v";
				declare $deleted_at as Timestamp;
				declare $disk_id as Utf8;
				declare $status as Int64;

				delete from disks
				where id = $disk_id and status = $status;

				delete from deleted
				where deleted_at = $deleted_at and disk_id = $disk_id
			`, s.disksPath), ydb_table.NewQueryParameters(
				ydb_table.ValueParam("$deleted_at", persistence.TimestampValue(deletedAt)),
				ydb_table.ValueParam("$disk_id", ydb_types.UTF8Value(diskID)),
				ydb_table.ValueParam("$status", ydb_types.Int64Value(int64(diskStatusDeleted))),
			))
			if err != nil {
				return err
			}
		}
	}

	return nil
}

func (s *storageYDB) listDisks(
	ctx context.Context,
	session *persistence.Session,
	folderID string,
) ([]string, error) {

	return listResources(ctx, session, s.disksPath, "disks", folderID)
}

////////////////////////////////////////////////////////////////////////////////

func (s *storageYDB) CreateDisk(
	ctx context.Context,
	disk DiskMeta,
) (*DiskMeta, error) {

	var created *DiskMeta

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			created, err = s.createDisk(ctx, session, disk)
			return err
		},
	)
	if err != nil {
		return nil, err
	}

	return created, nil
}

func (s *storageYDB) DiskCreated(
	ctx context.Context,
	disk DiskMeta,
) error {

	return s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			return s.diskCreated(ctx, session, disk)
		},
	)
}

func (s *storageYDB) GetDiskMeta(
	ctx context.Context,
	diskID string,
) (*DiskMeta, error) {

	var disk *DiskMeta

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			disk, err = s.getDisk(ctx, session, diskID)
			return err
		},
	)
	if err != nil {
		return nil, err
	}

	return disk, nil
}

func (s *storageYDB) DeleteDisk(
	ctx context.Context,
	diskID string,
	taskID string,
	deletingAt time.Time,
) (*DiskMeta, error) {

	var disk *DiskMeta

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			disk, err = s.deleteDisk(ctx, session, diskID, taskID, deletingAt)
			return err
		},
	)
	if err != nil {
		return nil, err
	}

	return disk, nil
}

func (s *storageYDB) DiskDeleted(
	ctx context.Context,
	diskID string,
	deletedAt time.Time,
) error {

	return s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			return s.diskDeleted(ctx, session, diskID, deletedAt)
		},
	)
}

func (s *storageYDB) ClearDeletedDisks(
	ctx context.Context,
	deletedBefore time.Time,
	limit int,
) error {

	return s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			return s.clearDeletedDisks(ctx, session, deletedBefore, limit)
		},
	)
}

func (s *storageYDB) ListDisks(
	ctx context.Context,
	folderID string,
) ([]string, error) {

	var ids []string

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			ids, err = s.listDisks(ctx, session, folderID)
			return err
		},
	)
	if err != nil {
		return nil, err
	}

	return ids, nil
}

////////////////////////////////////////////////////////////////////////////////

func createDisksYDBTables(
	ctx context.Context,
	folder string,
	db *persistence.YDBClient,
) error {

	logging.Info(ctx, "Creating tables for disks in %v", db.AbsolutePath(folder))

	err := db.CreateOrAlterTable(ctx, folder, "disks", diskStateTableDescription())
	if err != nil {
		return err
	}
	logging.Info(ctx, "Created disks table")

	err = db.CreateOrAlterTable(ctx, folder, "deleted", persistence.MakeCreateTableDescription(
		persistence.WithColumn("deleted_at", ydb_types.Optional(ydb_types.TypeTimestamp)),
		persistence.WithColumn("disk_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithPrimaryKeyColumn("deleted_at", "disk_id"),
	))
	if err != nil {
		return err
	}
	logging.Info(ctx, "Created deleted table")

	logging.Info(ctx, "Created tables for disks")

	return nil
}

func dropDisksYDBTables(
	ctx context.Context,
	folder string,
	db *persistence.YDBClient,
) error {

	logging.Info(ctx, "Dropping tables for disks in %v", db.AbsolutePath(folder))

	err := db.DropTable(ctx, folder, "disks")
	if err != nil {
		return err
	}
	logging.Info(ctx, "Dropped disks table")

	err = db.DropTable(ctx, folder, "deleted")
	if err != nil {
		return err
	}
	logging.Info(ctx, "Dropped deleted table")

	logging.Info(ctx, "Dropped tables for disks")

	return nil
}
