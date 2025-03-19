package provider

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	metadbmock "a.yandex-team.ru/cloud/mdb/backup/internal/metadb/mocks"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/models"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func newLogger(t *testing.T) log.Logger {
	l, err := zap.New(zap.KVConfig(log.DebugLevel))
	require.NoError(t, err)
	require.NotNil(t, l)
	return l
}

func TestJobProducer_ProduceBackupJob(t *testing.T) {
	lg := newLogger(t)

	type result struct {
		Backup metadb.Backup
		Error  error
	}

	tests := []struct {
		name         string
		ctx          context.Context
		mdbMockFunc  func(ctrl *gomock.Controller) *metadbmock.MockMetaDB
		produce      []result
		expectedJobs int
		expectedErr  error
	}{
		{
			name: "all_produced_job_passed",
			ctx:  context.TODO(),
			produce: []result{
				{metadb.Backup{}, nil},
				{metadb.Backup{}, nil},
				{metadb.Backup{}, nil},
			},
			mdbMockFunc: func(ctrl *gomock.Controller) *metadbmock.MockMetaDB {
				mdbMock := metadbmock.NewMockMetaDB(ctrl)
				mdbMock.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(context.TODO(), nil).Times(3)
				return mdbMock
			},
			expectedJobs: 3,
		},
		{
			name: "all_produced_job_passed,_errors_invoke_rollback",
			ctx:  context.TODO(),
			produce: []result{
				{metadb.Backup{}, nil},
				{metadb.Backup{}, nil},
				{metadb.Backup{}, xerrors.Errorf("produce func error")},
				{metadb.Backup{}, metadb.ErrDataNotFound},
				{metadb.Backup{}, nil},
			},
			mdbMockFunc: func(ctrl *gomock.Controller) *metadbmock.MockMetaDB {
				mdbMock := metadbmock.NewMockMetaDB(ctrl)
				mdbMock.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(context.TODO(), nil).Times(5)
				mdbMock.EXPECT().Rollback(gomock.Any()).Return(nil).Times(2)
				return mdbMock
			},
			expectedJobs: 3,
		},
		{
			name: "begin_returns_error,jobs_skipped",
			ctx:  context.TODO(),
			produce: []result{
				{metadb.Backup{}, nil},
				{metadb.Backup{}, xerrors.Errorf("txn begin error")},
			},
			mdbMockFunc: func(ctrl *gomock.Controller) *metadbmock.MockMetaDB {
				mdbMock := metadbmock.NewMockMetaDB(ctrl)
				gomock.InOrder(
					mdbMock.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(context.TODO(), nil),
					mdbMock.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(nil, xerrors.Errorf("txn begin error")),
				)
				return mdbMock
			},
			expectedJobs: 1,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)

			jp := &JobProducer{
				produceFunc: func(jobs []result) func(ctx context.Context) (metadb.Backup, error) {
					i := -1
					return func(ctx context.Context) (metadb.Backup, error) {
						i++
						return jobs[i].Backup, jobs[i].Error
					}
				}(tt.produce),
				mdb: tt.mdbMockFunc(ctrl),
				lg:  lg,
			}

			ch := make(chan models.BackupJob, len(tt.produce))
			for i := 0; i < len(tt.produce); i++ {
				job := tt.produce[i]
				lg.Debugf("i: %d, job: %+v\n", i, job)
				err := jp.ProduceBackupJob(tt.ctx, ch)

				if job.Error != nil {
					require.Error(t, err)
					require.EqualError(t, job.Error, err.Error())
				} else {
					require.NoError(t, err)
				}
			}
			close(ch)

			gotJobs := make([]models.BackupJob, 0)
			for j := range ch {
				gotJobs = append(gotJobs, j)
			}
			require.Len(t, gotJobs, tt.expectedJobs)

			ctrl.Finish()
		})
	}
}
