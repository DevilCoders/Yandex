package provider

import (
	"context"
	"fmt"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/jobproducer/mocks"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func newLogger(t *testing.T) log.Logger {
	l, err := zap.New(zap.KVConfig(log.DebugLevel))
	require.NoError(t, err)
	require.NotNil(t, l)
	return l
}

func TestQueueProducer_ProduceQueue(t *testing.T) {
	type fields struct {
		jobProducerMockFunc func(t *testing.T) (*mocks.MockJobProducer, *gomock.Controller)
		cfg                 Config
	}
	type args struct {
		ctx context.Context
	}
	lg := newLogger(t)

	tests := []struct {
		name        string
		fields      fields
		args        args
		expectedErr error
	}{
		{
			name: "exits_on_ctx_close",
			fields: fields{
				jobProducerMockFunc: func(t *testing.T) (*mocks.MockJobProducer, *gomock.Controller) {
					ctrl := gomock.NewController(t)
					jobProducerMock := mocks.NewMockJobProducer(ctrl)
					return jobProducerMock, ctrl
				},
			},

			args: args{
				ctx: func() context.Context {
					ctx, cancel := context.WithCancel(context.Background())
					cancel()
					return ctx
				}(),
			},
			expectedErr: nil,
		},
		{
			name: "exits_on_max_tasks_exhaust",
			fields: fields{
				jobProducerMockFunc: func(t *testing.T) (*mocks.MockJobProducer, *gomock.Controller) {
					ctrl := gomock.NewController(t)
					jobProducerMock := mocks.NewMockJobProducer(ctrl)
					jobProducerMock.EXPECT().ProduceBackupJob(gomock.Any(), gomock.Any()).Return(nil).Times(5)
					return jobProducerMock, ctrl
				},
				cfg: Config{MaxTasks: 5},
			},
			args: args{
				ctx: context.TODO(),
			},
			expectedErr: nil,
		},
		{
			name: "exits_on_unknown_error",
			fields: fields{
				jobProducerMockFunc: func(t *testing.T) (*mocks.MockJobProducer, *gomock.Controller) {
					ctrl := gomock.NewController(t)
					jobProducerMock := mocks.NewMockJobProducer(ctrl)
					jobProducerMock.EXPECT().ProduceBackupJob(gomock.Any(), gomock.Any()).Return(fmt.Errorf("unknown err"))
					return jobProducerMock, ctrl
				},
			},
			args: args{
				ctx: context.TODO(),
			},
			expectedErr: fmt.Errorf("can not fetch backup job: unknown err"),
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			jobProducer, ctrl := tt.fields.jobProducerMockFunc(t)
			qp := &QueueProducer{
				jobProducer: jobProducer,
				lg:          lg,
				cfg:         tt.fields.cfg,
			}
			outc, errc := qp.ProduceQueue(tt.args.ctx, tt.args.ctx)

			err := <-errc
			err2, ok := <-errc
			require.NoError(t, err2)
			require.False(t, ok)

			if tt.expectedErr != nil {
				require.EqualError(t, err, tt.expectedErr.Error())
				return
			}
			require.NoError(t, err)

			_, ok = <-outc
			require.False(t, ok)
			ctrl.Finish()
		})
	}
}
