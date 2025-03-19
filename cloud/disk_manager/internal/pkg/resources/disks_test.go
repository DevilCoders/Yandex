package resources

import (
	"context"
	"testing"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/wrappers"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

func requireDisksAreEqual(t *testing.T, expected DiskMeta, actual DiskMeta) {
	// TODO: Get rid of boilerplate.
	require.Equal(t, expected.ID, actual.ID)
	require.Equal(t, expected.ZoneID, actual.ZoneID)
	require.Equal(t, expected.SrcImageID, actual.SrcImageID)
	require.Equal(t, expected.SrcSnapshotID, actual.SrcSnapshotID)
	require.Equal(t, expected.BlocksCount, actual.BlocksCount)
	require.Equal(t, expected.BlockSize, actual.BlockSize)
	require.Equal(t, expected.Kind, actual.Kind)
	require.Equal(t, expected.CloudID, actual.CloudID)
	require.Equal(t, expected.FolderID, actual.FolderID)
	require.Equal(t, expected.PlacementGroupID, actual.PlacementGroupID)

	require.True(t, proto.Equal(expected.CreateRequest, actual.CreateRequest))
	require.Equal(t, expected.CreateTaskID, actual.CreateTaskID)
	if !expected.CreatingAt.IsZero() {
		require.WithinDuration(t, expected.CreatingAt, actual.CreatingAt, time.Microsecond)
	}
	if !expected.CreatedAt.IsZero() {
		require.WithinDuration(t, expected.CreatedAt, actual.CreatedAt, time.Microsecond)
	}
	require.Equal(t, expected.CreatedBy, actual.CreatedBy)
	require.Equal(t, expected.DeleteTaskID, actual.DeleteTaskID)
}

////////////////////////////////////////////////////////////////////////////////

func TestDisksCreateDisk(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	storage := createStorage(t, ctx, db)

	disk := DiskMeta{
		ID:               "disk",
		ZoneID:           "zone",
		SrcImageID:       "image",
		SrcSnapshotID:    "snapshot",
		BlocksCount:      1000000,
		BlockSize:        4096,
		Kind:             "ssd",
		CloudID:          "cloud",
		FolderID:         "folder",
		PlacementGroupID: "group",

		CreateRequest: &wrappers.UInt64Value{
			Value: 1,
		},
		CreateTaskID: "create",
		CreatingAt:   time.Now(),
		CreatedBy:    "user",
	}

	actual, err := storage.CreateDisk(ctx, disk)
	require.NoError(t, err)
	require.NotNil(t, actual)

	expected := disk
	expected.CreateRequest = nil
	requireDisksAreEqual(t, expected, *actual)

	// Check idempotency.
	actual, err = storage.CreateDisk(ctx, disk)
	require.NoError(t, err)
	require.NotNil(t, actual)

	disk.CreatedAt = time.Now()
	err = storage.DiskCreated(ctx, disk)
	require.NoError(t, err)

	// Check idempotency.
	err = storage.DiskCreated(ctx, disk)
	require.NoError(t, err)

	// Check idempotency.
	actual, err = storage.CreateDisk(ctx, disk)
	require.NoError(t, err)
	require.NotNil(t, actual)

	expected = disk
	expected.CreateRequest = nil
	requireDisksAreEqual(t, expected, *actual)

	disk.CreateTaskID = "other"
	actual, err = storage.CreateDisk(ctx, disk)
	require.NoError(t, err)
	require.Nil(t, actual)
}

func TestDisksDeleteDisk(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	storage := createStorage(t, ctx, db)

	disk := DiskMeta{
		ID: "disk",
		CreateRequest: &wrappers.UInt64Value{
			Value: 1,
		},
		CreateTaskID: "create",
		CreatingAt:   time.Now(),
		CreatedBy:    "user",
	}

	actual, err := storage.CreateDisk(ctx, disk)
	require.NoError(t, err)
	require.NotNil(t, actual)

	expected := disk
	expected.CreateRequest = nil
	expected.DeleteTaskID = "delete"

	actual, err = storage.DeleteDisk(ctx, disk.ID, "delete", time.Now())
	require.NoError(t, err)
	requireDisksAreEqual(t, expected, *actual)

	disk.CreatedAt = time.Now()
	err = storage.DiskCreated(ctx, disk)
	require.Error(t, err)
	require.True(t, errors.Is(err, &errors.NonRetriableError{}))

	// Check idempotency.
	actual, err = storage.DeleteDisk(ctx, disk.ID, "delete", time.Now())
	require.NoError(t, err)
	requireDisksAreEqual(t, expected, *actual)

	_, err = storage.CreateDisk(ctx, disk)
	require.Error(t, err)
	require.True(t, errors.Is(err, &errors.NonRetriableError{}))

	err = storage.DiskDeleted(ctx, disk.ID, time.Now())
	require.NoError(t, err)

	// Check idempotency.
	actual, err = storage.DeleteDisk(ctx, disk.ID, "delete", time.Now())
	require.NoError(t, err)
	requireDisksAreEqual(t, expected, *actual)

	_, err = storage.CreateDisk(ctx, disk)
	require.Error(t, err)
	require.True(t, errors.Is(err, &errors.NonRetriableError{}))

	err = storage.DiskCreated(ctx, disk)
	require.Error(t, err)
	require.True(t, errors.Is(err, &errors.NonRetriableError{}))
}

func TestDisksDeleteNonexistentDisk(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	storage := createStorage(t, ctx, db)

	disk := DiskMeta{
		ID:           "disk",
		DeleteTaskID: "delete",
	}

	err = storage.DiskDeleted(ctx, disk.ID, time.Now())
	require.NoError(t, err)

	created := disk
	created.CreatedAt = time.Now()
	err = storage.DiskCreated(ctx, created)
	require.Error(t, err)
	require.True(t, errors.Is(err, &errors.NonRetriableError{}))

	deletingAt := time.Now()
	actual, err := storage.DeleteDisk(ctx, disk.ID, "delete", deletingAt)
	require.NoError(t, err)
	requireDisksAreEqual(t, disk, *actual)

	// Check idempotency.
	deletingAt = deletingAt.Add(time.Second)
	actual, err = storage.DeleteDisk(ctx, disk.ID, "delete", deletingAt)
	require.NoError(t, err)
	requireDisksAreEqual(t, disk, *actual)

	_, err = storage.CreateDisk(ctx, disk)
	require.Error(t, err)
	require.True(t, errors.Is(err, &errors.NonRetriableError{}))

	err = storage.DiskDeleted(ctx, disk.ID, time.Now())
	require.NoError(t, err)
}

func TestDisksClearDeletedDisks(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	storage := createStorage(t, ctx, db)

	deletedAt := time.Now()
	deletedBefore := deletedAt.Add(-time.Microsecond)

	err = storage.ClearDeletedDisks(ctx, deletedBefore, 10)
	require.NoError(t, err)

	disk := DiskMeta{
		ID: "disk",
		CreateRequest: &wrappers.UInt64Value{
			Value: 1,
		},
		CreateTaskID: "create",
		CreatingAt:   time.Now(),
		CreatedBy:    "user",
	}

	actual, err := storage.CreateDisk(ctx, disk)
	require.NoError(t, err)
	require.NotNil(t, actual)

	_, err = storage.DeleteDisk(ctx, disk.ID, "delete", deletedAt)
	require.NoError(t, err)

	err = storage.DiskDeleted(ctx, disk.ID, deletedAt)
	require.NoError(t, err)

	err = storage.ClearDeletedDisks(ctx, deletedBefore, 10)
	require.NoError(t, err)

	_, err = storage.CreateDisk(ctx, disk)
	require.Error(t, err)
	require.True(t, errors.Is(err, &errors.NonRetriableError{}))

	deletedBefore = deletedAt.Add(time.Microsecond)
	err = storage.ClearDeletedDisks(ctx, deletedBefore, 10)
	require.NoError(t, err)

	actual, err = storage.CreateDisk(ctx, disk)
	require.NoError(t, err)
	require.NotNil(t, actual)
}

func TestDisksGetDisk(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	storage := createStorage(t, ctx, db)

	diskID := t.Name()

	d, err := storage.GetDiskMeta(ctx, diskID)
	require.NoError(t, err)
	require.Nil(t, d)

	disk := DiskMeta{
		ID:               diskID,
		ZoneID:           "zone",
		SrcImageID:       "image",
		SrcSnapshotID:    "snapshot",
		BlocksCount:      1000000,
		BlockSize:        4096,
		Kind:             "ssd",
		CloudID:          "cloud",
		FolderID:         "folder",
		PlacementGroupID: "group",

		CreateRequest: &wrappers.UInt64Value{
			Value: 1,
		},
		CreateTaskID: "create",
		CreatingAt:   time.Now(),
		CreatedBy:    "user",
	}

	actual, err := storage.CreateDisk(ctx, disk)
	require.NoError(t, err)
	require.NotNil(t, actual)

	disk.CreateRequest = nil

	d, err = storage.GetDiskMeta(ctx, diskID)
	require.NoError(t, err)
	require.NotNil(t, d)
	requireDisksAreEqual(t, disk, *d)
}
