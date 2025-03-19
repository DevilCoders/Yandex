package common

import (
	"context"
	"testing"

	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
)

////////////////////////////////////////////////////////////////////////////////

const (
	chunkSize = 4096
)

////////////////////////////////////////////////////////////////////////////////

type mockSource struct {
	mock.Mock
}

func (m *mockSource) Next(
	ctx context.Context,
	milestoneChunkIndex uint32,
	processedChunkIndices chan uint32,
) (chunkIndices <-chan uint32, errors <-chan error) {

	args := m.Called(ctx, milestoneChunkIndex, processedChunkIndices)
	return args.Get(0).(chan uint32), args.Get(1).(chan error)
}

func (m *mockSource) Read(
	ctx context.Context,
	chunk *Chunk,
) error {

	args := m.Called(ctx, chunk)
	return args.Error(0)
}

func (m *mockSource) GetMilestoneChunkIndex() uint32 {
	args := m.Called()
	return args.Get(0).(uint32)
}

func (m *mockSource) GetChunkCount(ctx context.Context) (uint32, error) {
	args := m.Called(ctx)
	return args.Get(0).(uint32), args.Error(1)
}

func (m *mockSource) Close(ctx context.Context) {
	m.Called(ctx)
}

////////////////////////////////////////////////////////////////////////////////

type mockTarget struct {
	mock.Mock
}

func (m *mockTarget) Write(
	ctx context.Context,
	chunk Chunk,
) error {

	args := m.Called(ctx, chunk)
	return args.Error(0)
}

func (m *mockTarget) Close(ctx context.Context) {
	m.Called(ctx)
}

////////////////////////////////////////////////////////////////////////////////

func createContext() context.Context {
	return logging.SetLogger(
		context.Background(),
		logging.CreateStderrLogger(logging.DebugLevel),
	)
}

////////////////////////////////////////////////////////////////////////////////

func TestTransfer(t *testing.T) {
	ctx := createContext()
	source := &mockSource{}
	target := &mockTarget{}

	chunks := make([]Chunk, 0)
	for i := 0; i < 101; i++ {
		chunkIndex := uint32(i * 17)
		chunk := Chunk{}

		source.On("Read", mock.Anything, chunk.Index, mock.Anything).Return(nil).Run(func(args mock.Arguments) {
			chunk := args.Get(2).(*Chunk)
			chunk.Index = chunkIndex
			target.On("Write", mock.Anything, *chunk).Return(nil)
			chunks = append(chunks, *chunk)
		})
	}

	milestoneChunkIndex := uint32(42)
	chunkIndices := make(chan uint32)
	chunkIndicesErrors := make(chan error)

	source.On("Next", mock.Anything, milestoneChunkIndex, mock.Anything).Return(
		chunkIndices,
		chunkIndicesErrors,
	).Run(func(args mock.Arguments) {
		go func() {
			processedChunkIndices := args.Get(2).(chan uint32)

			logging.Info(ctx, "Next called, processedChunkIndices=%v", processedChunkIndices)

			chunksInflight := 0
			for _, chunk := range chunks {
				chunkIndices <- chunk.Index

				chunksInflight++
				for chunksInflight > cap(processedChunkIndices) {
					<-processedChunkIndices
					chunksInflight--
				}
			}

			chunkIndicesErrors <- nil
			close(chunkIndices)
			close(chunkIndicesErrors)
		}()
	})

	err := Transfer(
		ctx,
		source,
		target,
		milestoneChunkIndex,
		func(context.Context) error { return nil },
		11,
		22,
		10,
		chunkSize,
	)
	require.NoError(t, err)
}
