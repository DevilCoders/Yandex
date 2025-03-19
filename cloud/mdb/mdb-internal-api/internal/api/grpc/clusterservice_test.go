package grpc

import (
	"context"
	"sync"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	grpcmocks "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	commonmocks "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/logs"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestClusterService_StreamLogs(t *testing.T) {
	t.Run("BatchError", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		logsMock := commonmocks.NewMockLogs(ctrl)
		cs := &ClusterService{Logs: logsMock, L: &nop.Logger{}}

		cid := "cid"
		lst := logs.ServiceTypeClickHouse
		reqMock := grpcmocks.NewMockStreamClusterLogsRequest(ctrl)
		reqMock.EXPECT().GetColumnFilter().Return(nil)
		reqMock.EXPECT().GetFromTime().Return(nil)
		reqMock.EXPECT().GetToTime().Return(nil)
		reqMock.EXPECT().GetRecordToken().Return("")
		reqMock.EXPECT().GetFilter().Return("")
		reqMock.EXPECT().GetClusterId().Return(cid)

		expected := xerrors.New("send failed")

		stopCtx, cancel := context.WithCancel(context.Background())
		var wg sync.WaitGroup
		logsMock.EXPECT().Stream(gomock.Any(), cid, lst, common.LogsOptions{}).
			DoAndReturn(func(ctx context.Context,
				cid string,
				st logs.ServiceType,
				opts common.LogsOptions,
			) (<-chan common.LogsBatch, error) {
				ch := make(chan common.LogsBatch)
				wg.Add(1)
				go func() {
					ch <- common.LogsBatch{Err: expected}
					select {
					case ch <- common.LogsBatch{Logs: []logs.Message{{Message: map[string]string{"key": "value"}}}}:
					case <-stopCtx.Done():
						t.Error("Context is canceled but second batch is still not sent. StreamLogs returned before verifying that sender closed the channel after sending the error.")
					}
					close(ch)
					wg.Done()
				}()
				return ch, nil
			})
		err := cs.StreamLogs(context.Background(), lst, reqMock, func(l logs.Message) error { return nil })
		require.Error(t, err)
		require.True(t, xerrors.Is(err, expected))

		cancel()
		wg.Wait()
	})

	t.Run("SendError", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		logsMock := commonmocks.NewMockLogs(ctrl)
		cs := &ClusterService{Logs: logsMock, L: &nop.Logger{}}

		cid := "cid"
		lst := logs.ServiceTypeClickHouse
		reqMock := grpcmocks.NewMockStreamClusterLogsRequest(ctrl)
		reqMock.EXPECT().GetColumnFilter().Return(nil)
		reqMock.EXPECT().GetFromTime().Return(nil)
		reqMock.EXPECT().GetToTime().Return(nil)
		reqMock.EXPECT().GetRecordToken().Return("")
		reqMock.EXPECT().GetFilter().Return("")
		reqMock.EXPECT().GetClusterId().Return(cid)

		stopCtx, cancel := context.WithCancel(context.Background())
		var wg sync.WaitGroup
		logsMock.EXPECT().Stream(gomock.Any(), cid, lst, common.LogsOptions{}).
			DoAndReturn(func(ctx context.Context,
				cid string,
				st logs.ServiceType,
				opts common.LogsOptions,
			) (<-chan common.LogsBatch, error) {
				ch := make(chan common.LogsBatch)
				wg.Add(1)
				go func() {
					ch <- common.LogsBatch{Logs: []logs.Message{{Message: map[string]string{"key1": "value1"}}}}
					select {
					case ch <- common.LogsBatch{Logs: []logs.Message{{Message: map[string]string{"key2": "value2"}}}}:
					case <-stopCtx.Done():
						t.Error("Context is canceled but second batch is still not sent. StreamLogs returned before all batches were drained and channel was closed.")
					}
					close(ch)
					wg.Done()
				}()
				return ch, nil
			})
		expected := xerrors.New("send failed")
		err := cs.StreamLogs(context.Background(), lst, reqMock, func(l logs.Message) error { return expected })
		require.Error(t, err)
		require.True(t, xerrors.Is(err, expected))

		cancel()
		wg.Wait()
	})

	t.Run("Panic", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		logsMock := commonmocks.NewMockLogs(ctrl)
		cs := &ClusterService{Logs: logsMock, L: &nop.Logger{}}

		cid := "cid"
		lst := logs.ServiceTypeClickHouse
		reqMock := grpcmocks.NewMockStreamClusterLogsRequest(ctrl)
		reqMock.EXPECT().GetColumnFilter().Return(nil)
		reqMock.EXPECT().GetFromTime().Return(nil)
		reqMock.EXPECT().GetToTime().Return(nil)
		reqMock.EXPECT().GetRecordToken().Return("")
		reqMock.EXPECT().GetFilter().Return("")
		reqMock.EXPECT().GetClusterId().Return(cid)

		stopCtx, cancel := context.WithCancel(context.Background())
		var wg sync.WaitGroup
		logsMock.EXPECT().Stream(gomock.Any(), cid, lst, common.LogsOptions{}).
			DoAndReturn(func(ctx context.Context,
				cid string,
				st logs.ServiceType,
				opts common.LogsOptions,
			) (<-chan common.LogsBatch, error) {
				ch := make(chan common.LogsBatch)
				wg.Add(1)
				go func() {
					ch <- common.LogsBatch{Logs: []logs.Message{{Message: map[string]string{"key1": "value1"}}}}
					select {
					case ch <- common.LogsBatch{Logs: []logs.Message{{Message: map[string]string{"key2": "value2"}}}}:
					case <-stopCtx.Done():
						t.Error("Context is canceled but second batch is still not sent. StreamLogs returned before all batches were drained and channel was closed.")
					}
					close(ch)
					wg.Done()
				}()
				return ch, nil
			})
		require.Panics(t, func() {
			_ = cs.StreamLogs(context.Background(), lst, reqMock, func(l logs.Message) error { panic("oh my god!") })
		})

		cancel()
		wg.Wait()
	})
}
