package mocks

import (
	"context"

	"github.com/stretchr/testify/mock"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/snapshot"
)

////////////////////////////////////////////////////////////////////////////////

type ClientMock struct {
	mock.Mock
}

func (c *ClientMock) Close() error {
	args := c.Called()
	return args.Error(0)
}

func (c *ClientMock) CreateImageFromURL(
	ctx context.Context,
	state snapshot.CreateImageFromURLState,
	saveState snapshot.SaveStateFunc,
) (snapshot.ResourceInfo, error) {

	args := c.Called(ctx, state, saveState)
	return args.Get(0).(snapshot.ResourceInfo), args.Error(1)
}

func (c *ClientMock) CreateImageFromImage(
	ctx context.Context,
	state snapshot.CreateImageFromImageState,
	saveState snapshot.SaveStateFunc,
) (snapshot.ResourceInfo, error) {

	args := c.Called(ctx, state, saveState)
	return args.Get(0).(snapshot.ResourceInfo), args.Error(1)
}

func (c *ClientMock) CreateImageFromSnapshot(
	ctx context.Context,
	state snapshot.CreateImageFromSnapshotState,
	saveState snapshot.SaveStateFunc,
) (snapshot.ResourceInfo, error) {

	args := c.Called(ctx, state, saveState)
	return args.Get(0).(snapshot.ResourceInfo), args.Error(1)
}

func (c *ClientMock) CreateImageFromDisk(
	ctx context.Context,
	state snapshot.CreateImageFromDiskState,
	saveState snapshot.SaveStateFunc,
) (snapshot.ResourceInfo, error) {

	args := c.Called(ctx, state, saveState)
	return args.Get(0).(snapshot.ResourceInfo), args.Error(1)
}

func (c *ClientMock) CreateSnapshotFromDisk(
	ctx context.Context,
	state snapshot.CreateSnapshotFromDiskState,
	saveState snapshot.SaveStateFunc,
) (snapshot.ResourceInfo, error) {

	args := c.Called(ctx, state, saveState)
	return args.Get(0).(snapshot.ResourceInfo), args.Error(1)
}

func (c *ClientMock) TransferFromImageToDisk(
	ctx context.Context,
	state snapshot.TransferFromImageToDiskState,
	saveState snapshot.SaveStateFunc,
) error {

	args := c.Called(ctx, state, saveState)
	return args.Error(0)
}

func (c *ClientMock) TransferFromSnapshotToDisk(
	ctx context.Context,
	state snapshot.TransferFromSnapshotToDiskState,
	saveState snapshot.SaveStateFunc,
) error {

	args := c.Called(ctx, state, saveState)
	return args.Error(0)
}

func (c *ClientMock) DeleteImage(
	ctx context.Context,
	state snapshot.DeleteImageState,
	saveState snapshot.SaveStateFunc,
) error {

	args := c.Called(ctx, state, saveState)
	return args.Error(0)
}

func (c *ClientMock) DeleteSnapshot(
	ctx context.Context,
	state snapshot.DeleteSnapshotState,
	saveState snapshot.SaveStateFunc,
) error {

	args := c.Called(ctx, state, saveState)
	return args.Error(0)
}

func (c *ClientMock) CheckResourceReady(
	ctx context.Context,
	resourceID string,
) (snapshot.ResourceInfo, error) {

	args := c.Called(ctx, resourceID)
	return args.Get(0).(snapshot.ResourceInfo), args.Error(1)
}

func (c *ClientMock) DeleteTask(ctx context.Context, taskID string) error {
	args := c.Called(ctx, taskID)
	return args.Error(0)
}

////////////////////////////////////////////////////////////////////////////////

func CreateClientMock() *ClientMock {
	return &ClientMock{}
}

////////////////////////////////////////////////////////////////////////////////

// Ensure that ClientMock implements Client.
func assertClientMockIsClient(arg *ClientMock) snapshot.Client {
	return arg
}
