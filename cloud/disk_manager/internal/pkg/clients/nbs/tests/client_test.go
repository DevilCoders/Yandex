package tests

import (
	"context"
	"fmt"
	"math/rand"
	"os"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/auth"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring/metrics"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/types"
)

////////////////////////////////////////////////////////////////////////////////

func createContext() context.Context {
	return logging.SetLogger(
		context.Background(),
		logging.CreateStderrLogger(logging.DebugLevel),
	)
}

////////////////////////////////////////////////////////////////////////////////

func createClientWithCreds(
	t *testing.T,
	ctx context.Context,
	creds auth.Credentials,
) nbs.Client {

	rootCertsFile := os.Getenv("DISK_MANAGER_RECIPE_ROOT_CERTS_FILE")

	factory, err := nbs.CreateFactoryWithCreds(
		ctx,
		&config.ClientConfig{
			Zones: map[string]*config.Zone{
				"zone": {
					Endpoints: []string{
						fmt.Sprintf(
							"localhost:%v",
							os.Getenv("DISK_MANAGER_RECIPE_NBS_PORT"),
						),
						fmt.Sprintf(
							"localhost:%v",
							os.Getenv("DISK_MANAGER_RECIPE_NBS_PORT"),
						),
					},
				},
			},
			RootCertsFile: &rootCertsFile,
		},
		creds,
		metrics.CreateEmptyRegistry(),
	)
	require.NoError(t, err)

	client, err := factory.GetClient(ctx, "zone")
	require.NoError(t, err)

	return client
}

func createClient(t *testing.T, ctx context.Context) nbs.Client {
	return createClientWithCreds(t, ctx, nil)
}

////////////////////////////////////////////////////////////////////////////////

type mockTokenProvider struct {
	mock.Mock
}

func (m *mockTokenProvider) Token(ctx context.Context) (string, error) {
	args := m.Called(ctx)
	return args.String(0), args.Error(1)
}

////////////////////////////////////////////////////////////////////////////////

func TestCreate(t *testing.T) {
	ctx := createContext()
	client := createClient(t, ctx)

	diskID := t.Name()

	err := client.Create(ctx, nbs.CreateDiskParams{
		ID:          diskID,
		BlocksCount: 10,
		BlockSize:   4096,
		Kind:        types.DiskKind_DISK_KIND_SSD,
	})
	require.NoError(t, err)

	// Creating the same disk twice is not an error
	err = client.Create(ctx, nbs.CreateDiskParams{
		ID:          diskID,
		BlocksCount: 10,
		BlockSize:   4096,
		Kind:        types.DiskKind_DISK_KIND_SSD,
	})
	require.NoError(t, err)
}

func TestDeleteDisk(t *testing.T) {
	ctx := createContext()
	client := createClient(t, ctx)

	diskID := t.Name()

	err := client.Create(ctx, nbs.CreateDiskParams{
		ID:          diskID,
		BlocksCount: 10,
		BlockSize:   4096,
		Kind:        types.DiskKind_DISK_KIND_SSD,
	})
	require.NoError(t, err)

	err = client.Delete(ctx, diskID)
	require.NoError(t, err)

	// Deleting the same disk twice is not an error.
	err = client.Delete(ctx, diskID)
	require.NoError(t, err)

	// Deleting non-existent disk is also not an error.
	err = client.Delete(ctx, diskID+"_does_not_exist")
	require.NoError(t, err)
}

func TestCreateDeleteCheckpoint(t *testing.T) {
	ctx := createContext()
	client := createClient(t, ctx)

	diskID := t.Name()

	err := client.Create(ctx, nbs.CreateDiskParams{
		ID:          diskID,
		BlocksCount: 10,
		BlockSize:   4096,
		Kind:        types.DiskKind_DISK_KIND_SSD,
	})
	require.NoError(t, err)

	err = client.CreateCheckpoint(ctx, diskID, "checkpointID")
	require.NoError(t, err)

	err = client.DeleteCheckpoint(ctx, diskID, "checkpointID")
	require.NoError(t, err)
}

func TestDeleteCheckpointOnUnexistingDisk(t *testing.T) {
	ctx := createContext()
	client := createClient(t, ctx)

	diskID := t.Name()

	err := client.DeleteCheckpoint(ctx, diskID, "checkpointID")
	require.NoError(t, err)
}

func TestDeleteCheckpointData(t *testing.T) {
	ctx := createContext()
	client := createClient(t, ctx)

	diskID := t.Name()

	err := client.Create(ctx, nbs.CreateDiskParams{
		ID:          diskID,
		BlocksCount: 10,
		BlockSize:   4096,
		Kind:        types.DiskKind_DISK_KIND_SSD,
	})
	require.NoError(t, err)

	err = client.DeleteCheckpointData(ctx, diskID, "checkpointID")
	require.NoError(t, err)

	err = client.CreateCheckpoint(ctx, diskID, "checkpointID")
	require.NoError(t, err)

	err = client.DeleteCheckpointData(ctx, diskID, "checkpointID")
	require.NoError(t, err)
}

func TestResizeDisk(t *testing.T) {
	ctx := createContext()
	client := createClient(t, ctx)

	diskID := t.Name()

	err := client.Create(ctx, nbs.CreateDiskParams{
		ID:          diskID,
		BlocksCount: 10,
		BlockSize:   4096,
		Kind:        types.DiskKind_DISK_KIND_SSD,
	})
	require.NoError(t, err)

	err = client.Resize(
		ctx,
		func() error { return nil },
		diskID,
		65536,
	)
	require.NoError(t, err)
}

func TestResizeDiskConcurrently(t *testing.T) {
	ctx := createContext()
	client := createClient(t, ctx)

	diskID := t.Name()

	err := client.Create(ctx, nbs.CreateDiskParams{
		ID:          diskID,
		BlocksCount: 10,
		BlockSize:   4096,
		Kind:        types.DiskKind_DISK_KIND_SSD,
	})
	require.NoError(t, err)

	errs := make(chan error)
	workers := 3

	for i := 0; i < workers; i++ {
		go func() {
			// TODO: Should not create new client, instead reuse the old one.
			client := createClient(t, ctx)
			errs <- client.Resize(
				ctx,
				func() error { return nil },
				diskID,
				65536,
			)
		}()
	}

	for i := 0; i < workers; i++ {
		err := <-errs
		if err != nil {
			assert.True(t, errors.Is(err, &errors.RetriableError{}))
		}
	}
}

func TestResizeDiskFailureBecauseSizeIsNotDivisibleByBlockSize(t *testing.T) {
	ctx := createContext()
	client := createClient(t, ctx)

	diskID := t.Name()

	err := client.Create(ctx, nbs.CreateDiskParams{
		ID:          diskID,
		BlocksCount: 10,
		BlockSize:   4096,
		Kind:        types.DiskKind_DISK_KIND_SSD,
	})
	require.NoError(t, err)

	err = client.Resize(
		ctx,
		func() error { return nil },
		diskID,
		65537,
	)
	require.Error(t, err)
}

func TestResizeDiskFailureBecauseSizeDecreaseIsForbidden(t *testing.T) {
	ctx := createContext()
	client := createClient(t, ctx)

	diskID := t.Name()

	err := client.Create(ctx, nbs.CreateDiskParams{
		ID:          diskID,
		BlocksCount: 10,
		BlockSize:   4096,
		Kind:        types.DiskKind_DISK_KIND_SSD,
	})
	require.NoError(t, err)

	err = client.Resize(
		ctx,
		func() error { return nil },
		diskID,
		20480,
	)
	require.Error(t, err)
}

func TestResizeDiskFailureWhileChekpointing(t *testing.T) {
	ctx := createContext()
	client := createClient(t, ctx)

	diskID := t.Name()

	err := client.Create(ctx, nbs.CreateDiskParams{
		ID:          diskID,
		BlocksCount: 10,
		BlockSize:   4096,
		Kind:        types.DiskKind_DISK_KIND_SSD,
	})
	require.NoError(t, err)

	err = client.Resize(
		ctx,
		func() error { return assert.AnError },
		diskID,
		65536,
	)
	require.Equal(t, assert.AnError, err)
}

func TestAlterDisk(t *testing.T) {
	ctx := createContext()
	client := createClient(t, ctx)

	diskID := t.Name()

	err := client.Create(ctx, nbs.CreateDiskParams{
		ID:          diskID,
		BlocksCount: 10,
		BlockSize:   4096,
		Kind:        types.DiskKind_DISK_KIND_SSD,
		CloudID:     "cloud",
		FolderID:    "folder",
	})
	require.NoError(t, err)

	err = client.Alter(
		ctx,
		func() error { return nil },
		diskID,
		"newCloud",
		"newFolder",
	)
	require.NoError(t, err)
}

func TestAlterDiskConcurrently(t *testing.T) {
	ctx := createContext()
	client := createClient(t, ctx)

	diskID := t.Name()

	err := client.Create(ctx, nbs.CreateDiskParams{
		ID:          diskID,
		BlocksCount: 10,
		BlockSize:   4096,
		Kind:        types.DiskKind_DISK_KIND_SSD,
		CloudID:     "cloud",
		FolderID:    "folder",
	})
	require.NoError(t, err)

	errs := make(chan error)
	workers := 3

	for i := 0; i < workers; i++ {
		go func() {
			// TODO: Should not create new client, instead reuse the old one.
			client := createClient(t, ctx)
			errs <- client.Alter(
				ctx,
				func() error { return nil },
				diskID,
				"newCloud",
				"newFolder",
			)
		}()
	}

	for i := 0; i < workers; i++ {
		err := <-errs
		if err != nil {
			assert.True(t, errors.Is(err, &errors.RetriableError{}))
		}
	}
}

func TestAlterDiskFailureWhileCheckpointing(t *testing.T) {
	ctx := createContext()
	client := createClient(t, ctx)

	diskID := t.Name()

	err := client.Create(ctx, nbs.CreateDiskParams{
		ID:          diskID,
		BlocksCount: 10,
		BlockSize:   4096,
		Kind:        types.DiskKind_DISK_KIND_SSD,
		CloudID:     "cloud",
		FolderID:    "folder",
	})
	require.NoError(t, err)

	err = client.Alter(
		ctx,
		func() error { return assert.AnError },
		diskID,
		"newCloud",
		"newFolder",
	)
	require.Equal(t, assert.AnError, err)
}

func TestRebaseDisk(t *testing.T) {
	ctx := createContext()
	client := createClient(t, ctx)

	diskID := t.Name()

	err := client.Create(ctx, nbs.CreateDiskParams{
		ID:          "base",
		BlocksCount: 10,
		BlockSize:   4096,
		Kind:        types.DiskKind_DISK_KIND_SSD,
	})
	require.NoError(t, err)

	err = client.Create(ctx, nbs.CreateDiskParams{
		ID:                   diskID,
		BaseDiskID:           "base",
		BaseDiskCheckpointID: "checkpoint",
		BlocksCount:          10,
		BlockSize:            4096,
		Kind:                 types.DiskKind_DISK_KIND_SSD,
		CloudID:              "cloud",
		FolderID:             "folder",
	})
	require.NoError(t, err)

	err = client.Create(ctx, nbs.CreateDiskParams{
		ID:          "newBase",
		BlocksCount: 10,
		BlockSize:   4096,
		Kind:        types.DiskKind_DISK_KIND_SSD,
	})
	require.NoError(t, err)

	err = client.Rebase(
		ctx,
		func() error { return nil },
		diskID,
		"base",
		"newBase",
	)
	require.NoError(t, err)

	// Check idempotency.
	err = client.Rebase(
		ctx,
		func() error { return nil },
		diskID,
		"base",
		"newBase",
	)
	require.NoError(t, err)

	err = client.Rebase(
		ctx,
		func() error { return nil },
		diskID,
		"base",
		"otherBase",
	)
	require.Error(t, err)
	require.True(t, errors.Is(err, &errors.NonRetriableError{}))
	require.ErrorContains(t, err, "unexpected")
}

func TestAssignDisk(t *testing.T) {
	ctx := createContext()
	client := createClient(t, ctx)

	diskID := t.Name()

	err := client.Create(ctx, nbs.CreateDiskParams{
		ID:          diskID,
		BlocksCount: 10,
		BlockSize:   4096,
		Kind:        types.DiskKind_DISK_KIND_SSD,
	})
	require.NoError(t, err)

	err = client.Assign(ctx, nbs.AssignDiskParams{
		ID:         diskID,
		InstanceID: "InstanceID",
		Token:      "Token",
		Host:       "Host",
	})
	require.NoError(t, err)
}

func TestUnassignDisk(t *testing.T) {
	ctx := createContext()
	client := createClient(t, ctx)

	diskID := t.Name()

	err := client.Create(ctx, nbs.CreateDiskParams{
		ID:          diskID,
		BlocksCount: 10,
		BlockSize:   4096,
		Kind:        types.DiskKind_DISK_KIND_SSD,
	})
	require.NoError(t, err)

	err = client.Unassign(ctx, diskID)
	require.NoError(t, err)
}

func TestUnassignDeletedDisk(t *testing.T) {
	ctx := createContext()
	client := createClient(t, ctx)

	diskID := t.Name()

	err := client.Create(ctx, nbs.CreateDiskParams{
		ID:          diskID,
		BlocksCount: 10,
		BlockSize:   4096,
		Kind:        types.DiskKind_DISK_KIND_SSD,
	})
	require.NoError(t, err)

	err = client.Delete(ctx, diskID)
	require.NoError(t, err)

	err = client.Unassign(ctx, diskID)
	require.NoError(t, err)
}

func TestTokenErrorsShouldBeRetriable(t *testing.T) {
	ctx := createContext()
	mockTokenProvider := &mockTokenProvider{}
	client := createClientWithCreds(t, ctx, mockTokenProvider)

	mockTokenProvider.On("Token", mock.Anything).Return("", assert.AnError).Times(10)
	mockTokenProvider.On("Token", mock.Anything).Return("", nil)

	err := client.Delete(ctx, "disk")
	require.NoError(t, err)
}

func TestGetCheckpointSize(t *testing.T) {
	ctx := createContext()
	client := createClient(t, ctx)

	diskID := t.Name()
	blockCount := uint64(1 << 30)
	blockSize := uint32(4096)

	err := client.Create(ctx, nbs.CreateDiskParams{
		ID:          diskID,
		BlocksCount: blockCount,
		BlockSize:   blockSize,
		Kind:        types.DiskKind_DISK_KIND_SSD,
	})
	require.NoError(t, err)

	session, err := client.MountRW(ctx, diskID)
	require.NoError(t, err)
	defer session.Close(ctx)

	maxUsedBlockIndex := uint64(0)
	rand.Seed(time.Now().UnixNano())

	// Set some blocks at the beginning.
	for i := uint64(0); i < 1024; i++ {
		bytes := make([]byte, 4096)
		rand.Read(bytes)

		if rand.Intn(2) == 0 {
			err = session.Write(ctx, i, bytes)
			require.NoError(t, err)

			maxUsedBlockIndex = i
		}
	}

	// Set some blocks at the tail.
	for i := blockCount - 1024; i < blockCount; i++ {
		bytes := make([]byte, 4096)
		rand.Read(bytes)

		if rand.Intn(2) == 0 {
			err = session.Write(ctx, i, bytes)
			require.NoError(t, err)

			maxUsedBlockIndex = i
		}
	}

	err = client.CreateCheckpoint(ctx, diskID, "checkpoint")
	require.NoError(t, err)

	var checkpointSize uint64
	err = client.GetCheckpointSize(
		ctx,
		func(blockIndex uint64, result uint64) error {
			checkpointSize = result
			return nil
		},
		diskID,
		"checkpoint",
		0, // milestoneBlockIndex
		0, // milestoneCheckpointSize
	)
	require.NoError(t, err)
	require.Equal(t, maxUsedBlockIndex*uint64(blockSize), checkpointSize)
}

func TestIsNotFoundError(t *testing.T) {
	ctx := createContext()
	client := createClient(t, ctx)

	_, err := client.Describe(ctx, "unexisting")
	require.Error(t, err)
	require.True(t, nbs.IsNotFoundError(err))

	// Should work even if error is wrapped.
	err = &errors.NonRetriableError{Err: err}
	require.True(t, nbs.IsNotFoundError(err))
}
